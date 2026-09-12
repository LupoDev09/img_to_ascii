//
// Created by lupo on 12.09.26.
//

#ifndef IMG_TO_ASCII_RAWOUTPUTGUARD_HPP
#define IMG_TO_ASCII_RAWOUTPUTGUARD_HPP

#if defined(__linux__) || defined(__APPLE__)
    #include <termios.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
#endif

class RawOutputGuard {
public:
    RawOutputGuard() {
#if defined(__linux__) || defined(__APPLE__)
        m_active = (tcgetattr(STDOUT_FILENO, &m_original) == 0);
        if (!m_active) return;  // kein TTY, z.B. Pipe/Datei -> nichts tun

        struct termios raw = m_original;
        raw.c_oflag &= ~OPOST;
        tcsetattr(STDOUT_FILENO, TCSANOW, &raw);

#elif defined(_WIN32)
        m_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        m_active = GetConsoleMode(m_handle, &m_original);
        if (!m_active) return;  // umgeleitet, kein Konsolen-Handle

        DWORD raw = m_original & ~ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(m_handle, raw);
#endif
    }

    ~RawOutputGuard() {
        if (!m_active) return;
#if defined(__linux__) || defined(__APPLE__)
        tcsetattr(STDOUT_FILENO, TCSANOW, &m_original);
#elif defined(_WIN32)
        SetConsoleMode(m_handle, m_original);
#endif
    }

    // nicht kopierbar/verschiebbar - es gibt nur einen stdout
    RawOutputGuard(const RawOutputGuard&) = delete;
    RawOutputGuard& operator=(const RawOutputGuard&) = delete;

private:
    bool m_active = false;
#if defined(__linux__) || defined(__APPLE__)
    struct termios m_original{};
#elif defined(_WIN32)
    HANDLE m_handle = nullptr;
    DWORD m_original = 0;
#endif
};

#endif// IMG_TO_ASCII_RAWOUTPUTGUARD_HPP
