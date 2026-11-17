/// @file
/// @brief 单实例运行守卫 Guard 与二次启动门铃的回调类型。
///
/// 同一个用户、同一个 key 在同一时刻只允许一个进程持有独占锁；主实例
/// 可以另外启用门铃，接收二次实例发出的启动通知。

#ifndef SINGLEPROC_SINGLEPROC_HPP
#define SINGLEPROC_SINGLEPROC_HPP

#include <cstdint>
#include <functional>
#include <string>

namespace singleproc
{

/// @brief 二次启动通知的回调类型。
///
/// 回调在调用 process_doorbell() 的线程中同步执行，必须保持简短。
using DoorbellHandler = std::function<void()>;

/// @brief 单实例运行守卫。
///
/// 构造时尝试取得进程级独占锁；一个进程持有锁期间，同 key 的其它进程
/// 会得到 AlreadyRunning。锁由文件锁承担，进程退出或异常终止后由内核释放。
///
/// 守卫不删除锁文件，因为删除锁文件会与其它进程产生 inode 竞争；
/// 锁文件长期存在不代表有实例在运行。
///
/// 主实例还可以启用二次启动门铃：二次实例在得到 AlreadyRunning 后向
/// 主实例发送一次无载荷通知，主实例通过 process_doorbell() 分发回调。
/// 门铃失败不会改变 state() 或 error_code()。
///
/// @note Guard 的方法不是线程安全的；门铃回调也不在库内部线程中执行。
class Guard
{
public:
    /// @brief 启动判定的结果。
    enum class State
    {
        /// @brief 当前进程取得独占权。
        Primary,

        /// @brief 同一个 key 已有进程在运行。
        AlreadyRunning,

        /// @brief key、目录或锁文件不可用。
        Error
    };

    /// @brief 尝试取得 key 对应的独占锁。
    ///
    /// 失败通过 state() 和 error_code() 报告：key 非法、目录含 NUL，或锁
    /// 文件不是普通文件时得到 EINVAL；锁文件属主不是当前用户时得到 EACCES；
    /// 路径过长时得到 ENAMETOOLONG；其余系统调用失败带回对应的 errno。
    /// 同一进程对同一个 key 再建一个 Guard 也会得到 AlreadyRunning。
    ///
    /// 门铃 socket 路径超出平台限制，或 bind、listen 失败时只禁用门铃，
    /// 独占锁仍然有效。
    ///
    /// @param key 应用唯一名，只允许字母、数字、'.'、'_'、'-'。
    /// @param runtime_dir 锁文件和门铃 socket 的目录；为空时使用
    ///     XDG_RUNTIME_DIR，再退回 /tmp。
    explicit Guard(const std::string& key, const std::string& runtime_dir = std::string());

    /// @brief 关闭门铃监听、删除 socket 文件并释放独占锁。
    ~Guard();

    /// @brief 禁用拷贝和移动：锁与 socket 的生命周期只跟随这个对象。
    ///
    /// 解锁和 socket 清理都绑定在对象生命周期上，复制或移动会让所有权
    /// 变得含糊。
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&&) = delete;
    Guard& operator=(Guard&&) = delete;

    /// @brief 返回启动判定的结果。
    ///
    /// @return Primary、AlreadyRunning 或 Error。
    State state() const noexcept;

    /// @brief 当前进程是否取得独占权。
    ///
    /// Error 也返回 false；需要区分“已有实例”和“锁失败”时请查 state()。
    ///
    /// @return Primary 时为 true，其余情况为 false。
    bool is_primary() const noexcept;

    /// @brief 返回 Error 时的 errno；正常或 AlreadyRunning 时为 0。
    ///
    /// @return Error 状态下的 errno；其余状态为 0。
    int error_code() const noexcept;

    /// @brief 注册二次启动通知的回调。
    ///
    /// 重复调用时后设置的回调生效。回调为空时保留待处理通知，等待之后的
    /// process_doorbell() 再分发。
    ///
    /// @param handler 通知到达后执行的函数；传空表示暂停分发。
    void set_doorbell_handler(DoorbellHandler handler);

    /// @brief 处理待处理的二次启动通知。
    ///
    /// timeout_ms 为 0 时非阻塞；小于 0 时等待到有通知为止，被信号中断时
    /// 继续等待。
    /// 门铃不可用时直接返回 false；收到通知时若 handler 为空，也不执行回调，
    /// 通知保留到调用方设置 handler 之后再分发。
    /// 多个连接会被合并，一次调用最多执行一次回调。
    /// handler 抛出异常时通知已消费，不会重放。
    /// 门铃与文件锁互不影响：门铃失败不会把 Primary 变成 Error。
    ///
    /// @param timeout_ms 最长等待毫秒数；0 表示不等待，负数表示无限等待。
    /// @return true 表示至少执行了一次回调。
    bool process_doorbell(int timeout_ms = 0);

    /// @brief 返回门铃监听 fd；门铃不可用时返回 -1。
    ///
    /// 这是借用 fd：归 Guard 所有，有效至 Guard 析构；调用方不得 close
    /// 或修改它。可以交给 poll、select 或 Qt QSocketNotifier 监听；
    /// fd 可读时再调用 process_doorbell(0)，不要直接读取该 fd。
    ///
    /// @return 门铃监听 fd；门铃未启用、创建失败或平台为 Windows 时为 -1。
    /// @note Windows 上用轮询门铃文件实现通知，没有可交给 poll/select 的 fd。
    int doorbell_fd() const noexcept;

private:
    /// @brief 启动判定的结果。
    State state_ = State::Error;

    /// @brief Error 状态下的 errno；其余状态为 0。
    int error_code_ = 0;

#ifdef _WIN32
    /// @brief 锁文件句柄；未取得独占锁时为空。
    void* lock_handle_ = nullptr;

    /// @brief 门铃文件路径；为空表示门铃不可用。
    std::string bell_path_;
#else
    /// @brief 锁文件 fd；未取得独占锁时为 -1。
    int fd_ = -1;

    /// @brief 门铃 socket 的路径；socket 地址不可用时为空。
    std::string socket_path_;

    /// @brief 门铃监听 fd；门铃未启用或创建失败时为 -1。
    int doorbell_fd_ = -1;
#endif

    /// @brief 已注册的二次启动回调。
    DoorbellHandler doorbell_handler_;

    /// @brief 已接入连接但尚未分发的通知。
    bool doorbell_pending_ = false;
};

} // namespace singleproc

#endif // SINGLEPROC_SINGLEPROC_HPP
