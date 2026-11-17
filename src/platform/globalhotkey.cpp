#include "platform/globalhotkey.h"

#include <QList>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <stacklog/stacklog.hpp>

namespace
{
constexpr int hotkeyTogglePet = 0xDC01;
constexpr int hotkeyOpenChat = 0xDC02;
constexpr int hotkeyToggleMute = 0xDC03;
} // namespace

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
{
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterAll();
}

bool GlobalHotkey::registerDefaultHotkeys(QString *error)
{
#ifdef Q_OS_WIN
    unregisterAll();
    const UINT modifiers = MOD_CONTROL | MOD_ALT;
    struct Binding
    {
        int id;
        UINT vk;
        const char *name;
    };
    const Binding bindings[] = {
        {hotkeyTogglePet, 'P', "Ctrl+Alt+P"},
        {hotkeyOpenChat, 'C', "Ctrl+Alt+C"},
        {hotkeyToggleMute, 'M', "Ctrl+Alt+M"},
    };
    for (const Binding &binding : bindings) {
        if (!RegisterHotKey(nullptr, binding.id, modifiers, binding.vk)) {
            STACKLOG_WARN() << "RegisterHotKey failed: " << binding.name;
            if (error != nullptr)
                *error = QStringLiteral("全局快捷键注册失败：%1").arg(QString::fromLatin1(binding.name));
            continue;
        }
        m_ids.append(binding.id);
    }
    STACKLOG_INFO() << "global hotkeys registered: " << m_ids.size();
    return !m_ids.isEmpty();
#else
    Q_UNUSED(error)
    return false;
#endif
}

void GlobalHotkey::unregisterAll()
{
#ifdef Q_OS_WIN
    for (int id : std::as_const(m_ids))
        UnregisterHotKey(nullptr, id);
#endif
    m_ids.clear();
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)
#ifdef Q_OS_WIN
    auto *msg = static_cast<MSG *>(message);
    if (msg == nullptr || msg->message != WM_HOTKEY)
        return false;
    switch (static_cast<int>(msg->wParam)) {
    case hotkeyTogglePet:
        emit togglePetRequested();
        return true;
    case hotkeyOpenChat:
        emit openChatRequested();
        return true;
    case hotkeyToggleMute:
        emit toggleMuteRequested();
        return true;
    default:
        break;
    }
#else
    Q_UNUSED(message)
#endif
    return false;
}
