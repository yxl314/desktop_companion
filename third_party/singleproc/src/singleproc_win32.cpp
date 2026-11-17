#include <singleproc/singleproc.hpp>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <string>
#include <utility>

#include <windows.h>

// Windows 后端：用独占共享模式打开锁文件实现单实例，用门铃文件实现二次启动通知。
//
// 语义与 POSIX 实现保持一致：
// - 构造时争抢锁文件；失败得到 AlreadyRunning 并尽力通知主实例；
// - 锁由进程退出（句柄关闭）释放，锁文件不删除；
// - 门铃是 best-effort，失败不影响 state()；
// - 门铃没有可交给 poll/select 的句柄，doorbell_fd() 返回 -1，
//   回调通过 process_doorbell() 轮询分发。

namespace
{

/// 门铃轮询间隔（毫秒）。
constexpr DWORD doorbell_poll_interval_ms = 15;

/// 路径长度上限；Windows 上远超实际需要，只为拒绝明显异常的输入。
constexpr std::size_t max_path_length = 32767;

/// 把 UTF-8 字符串转换成宽字符串。
std::wstring to_wide(const std::string& text)
{
    if (text.empty())
    {
        return {};
    }
    const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
    {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    (void)::MultiByteToWideChar(CP_UTF8,
                                0,
                                text.data(),
                                static_cast<int>(text.size()),
                                &wide[0],
                                length);
    return wide;
}

/// 把宽字符串转换成 UTF-8 字符串。
std::string to_utf8(const std::wstring& text)
{
    if (text.empty())
    {
        return {};
    }
    const int length = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
    {
        return {};
    }
    std::string utf8(static_cast<std::size_t>(length), '\0');
    (void)::WideCharToMultiByte(CP_UTF8,
                                0,
                                text.data(),
                                static_cast<int>(text.size()),
                                &utf8[0],
                                length,
                                nullptr,
                                nullptr);
    return utf8;
}

/// 读取指定的环境变量；不存在时返回空字符串。
std::string environment_value(const wchar_t* name)
{
    wchar_t buffer[32768];
    const DWORD length = ::GetEnvironmentVariableW(name, buffer, static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));
    if (length == 0 || length >= static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])))
    {
        return {};
    }
    return to_utf8(std::wstring(buffer, length));
}

/// 返回默认锁目录：优先 LOCALAPPDATA，其次 TEMP，最后当前目录。
std::string default_runtime_dir()
{
    std::string value = environment_value(L"LOCALAPPDATA");
    if (value.empty())
    {
        value = environment_value(L"TEMP");
    }
    return value;
}

/// 返回当前用户名，过滤成可安全用于文件名的形式。
std::string user_suffix()
{
    wchar_t buffer[256];
    DWORD length = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    std::wstring name;
    if (::GetUserNameW(buffer, &length) != 0 && length > 0)
    {
        name.assign(buffer, length > 0 ? length - 1 : 0);
    }

    std::string result;
    for (char value : to_utf8(name))
    {
        const bool alpha = (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
        const bool digit = value >= '0' && value <= '9';
        if (alpha || digit || value == '.' || value == '_' || value == '-')
        {
            result.push_back(value);
        }
        else
        {
            result.push_back('_');
        }
    }
    if (result.empty())
    {
        result = "user";
    }
    return result;
}

/// 判断 key 是否能安全地作为文件名的一部分。
bool valid_key(const std::string& key)
{
    if (key.empty())
    {
        return false;
    }
    return std::all_of(key.begin(),
                       key.end(),
                       [](char value)
                       {
                           const bool alpha = (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
                           const bool digit = value >= '0' && value <= '9';
                           return alpha || digit || value == '.' || value == '_' || value == '-';
                       });
}

/// 目录字符串中不能嵌入 NUL，否则路径会被系统调用静默截断。
bool valid_directory(const std::string& directory)
{
    return directory.find('\0') == std::string::npos;
}

/// 拼接目录和文件名；目录为空时使用默认目录。
std::string join_path(const std::string& directory, const std::string& name)
{
    std::string base = directory;
    if (base.empty())
    {
        base = default_runtime_dir();
    }
    if (base.empty())
    {
        base = ".";
    }
    if (base[base.size() - 1] != '\\' && base[base.size() - 1] != '/')
    {
        base += '\\';
    }
    return base + name;
}

/// 生成运行时文件路径；锁文件与门铃文件共用同一套命名规则。
std::string make_runtime_path(const std::string& directory, const std::string& key, const char* suffix)
{
    const std::string name = "singleproc-" + user_suffix() + "-" + key + suffix;
    return join_path(directory, name);
}

/// 生成锁文件路径。
std::string make_lock_path(const std::string& directory, const std::string& key)
{
    return make_runtime_path(directory, key, ".lock");
}

/// 生成门铃文件路径。
std::string make_bell_path(const std::string& directory, const std::string& key)
{
    return make_runtime_path(directory, key, ".bell");
}

/// 文件是否存在。
bool file_exists(const std::string& path)
{
    const DWORD attributes = ::GetFileAttributesW(to_wide(path).c_str());
    return attributes != INVALID_FILE_ATTRIBUTES;
}

/// 把 Win32 错误码映射成 errno 风格的值，保持 error_code() 的语义。
int map_win32_error(DWORD error)
{
    switch (error)
    {
    case ERROR_ACCESS_DENIED:
        return EACCES;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_INVALID_NAME:
    case ERROR_INVALID_PARAMETER:
        return EINVAL;
    case ERROR_BUFFER_OVERFLOW:
    case ERROR_FILENAME_EXCED_RANGE:
        return ENAMETOOLONG;
    default:
        return EIO;
    }
}

/// 二次实例：把门铃文件写到主实例可见的位置，通知是 best-effort。
void notify_doorbell(const std::string& bell_path)
{
    if (bell_path.empty())
    {
        return;
    }
    const std::wstring wide = to_wide(bell_path);
    HANDLE file = ::CreateFileW(wide.c_str(),
                                GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }
    const char marker = 1;
    DWORD written = 0;
    (void)::WriteFile(file, &marker, 1, &written, nullptr);
    (void)::CloseHandle(file);
}

/// 主实例：消费一个门铃文件；存在即表示至少发生了一次二次启动。
bool consume_doorbell(const std::string& bell_path)
{
    if (bell_path.empty() || !file_exists(bell_path))
    {
        return false;
    }
    (void)::DeleteFileW(to_wide(bell_path).c_str());
    return true;
}

} // namespace

namespace singleproc
{

Guard::Guard(const std::string& key, const std::string& runtime_dir)
{
    if (!valid_key(key) || !valid_directory(runtime_dir))
    {
        error_code_ = EINVAL;
        return;
    }

    const std::string lock_path = make_lock_path(runtime_dir, key);
    if (lock_path.size() >= max_path_length)
    {
        error_code_ = ENAMETOOLONG;
        return;
    }

    bell_path_ = make_bell_path(runtime_dir, key);

    // dwShareMode = 0：只要已有进程持有这个锁文件，打开就会失败。
    HANDLE handle = ::CreateFileW(to_wide(lock_path).c_str(),
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  nullptr,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  nullptr);
    if (handle == INVALID_HANDLE_VALUE)
    {
        const DWORD error = ::GetLastError();
        if (error == ERROR_SHARING_VIOLATION)
        {
            state_ = State::AlreadyRunning;
            error_code_ = 0;
            notify_doorbell(bell_path_);
        }
        else
        {
            error_code_ = map_win32_error(error);
        }
        return;
    }

    lock_handle_ = handle;
    state_ = State::Primary;
    error_code_ = 0;

    // 上次异常终止可能留下门铃文件；主实例启动时清理，避免误报。
    if (!bell_path_.empty())
    {
        (void)::DeleteFileW(to_wide(bell_path_).c_str());
    }
}

Guard::~Guard()
{
    if (lock_handle_ != nullptr)
    {
        (void)::CloseHandle(static_cast<HANDLE>(lock_handle_));
        lock_handle_ = nullptr;
    }

    // 二次实例刚写过门铃文件，不能在这里删掉；只有主实例退出时清理。
    if (state_ == State::Primary && !bell_path_.empty())
    {
        (void)::DeleteFileW(to_wide(bell_path_).c_str());
    }
    bell_path_.clear();
}

Guard::State Guard::state() const noexcept
{
    return state_;
}

bool Guard::is_primary() const noexcept
{
    return state_ == State::Primary;
}

int Guard::error_code() const noexcept
{
    return error_code_;
}

void Guard::set_doorbell_handler(DoorbellHandler handler)
{
    doorbell_handler_ = std::move(handler);
}

bool Guard::process_doorbell(int timeout_ms)
{
    if (!doorbell_handler_)
    {
        // 没有回调时保留待处理状态，等调用方设置回调后再分发。
        if (doorbell_pending_)
        {
            return false;
        }
    }
    else if (doorbell_pending_)
    {
        const DoorbellHandler handler = doorbell_handler_;
        doorbell_pending_ = false;
        handler();
        return true;
    }

    if (state_ != State::Primary)
    {
        return false;
    }

    const ULONGLONG start = ::GetTickCount64();
    for (;;)
    {
        if (consume_doorbell(bell_path_))
        {
            if (doorbell_handler_)
            {
                const DoorbellHandler handler = doorbell_handler_;
                handler();
                return true;
            }
            doorbell_pending_ = true;
            return false;
        }

        if (timeout_ms == 0)
        {
            return false;
        }
        if (timeout_ms > 0 && static_cast<long long>(::GetTickCount64() - start) >= timeout_ms)
        {
            return false;
        }
        ::Sleep(doorbell_poll_interval_ms);
    }
}

int Guard::doorbell_fd() const noexcept
{
    // Windows 没有 poll/select 用的门铃句柄；调用方改用 process_doorbell(0) 轮询。
    return -1;
}

} // namespace singleproc
