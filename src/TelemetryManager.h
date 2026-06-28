#pragma once
#include <string>
#include <vector>
#include <windows.h>

enum RegTweakType { REG_TWEAK_DWORD, REG_TWEAK_SZ };

struct RegistryTweak {
    HKEY         rootKey;
    std::wstring subKey;
    std::wstring valueName;
    RegTweakType type;
    DWORD        dwordData;
    std::wstring stringData;
    std::string  description;
};

class TelemetryManager {
public:
    static const std::vector<RegistryTweak>& GetRegistryTweaks();
    static void List();
    static void ApplyAll();
    // Restore registry values snapshotted before ApplyAll (option 15).
    // Reads the per-value backup written to %ProgramData%\Debloat\reg_backup.txt
    // and writes each original value back (or deletes the value if it did not
    // exist before ApplyAll overwrote it).
    static void Revert();

    // -- Backup format pure helpers ------------------------------------------
    // These parse and serialize the pipe-delimited registry backup file
    // written by ApplyAll() and read by Revert(). Public static so unit tests
    // can exercise them directly (mirrors the HostsManager pattern).
    //
    // Escape backslash and pipe so a field is safe inside the pipe-delimited
    // record format.
    static std::string EscapeField(const std::string& s);
    // Splits a line on unescaped '|'. '\|' -> '|', '\\' -> '\' in output.
    // An unknown escape (e.g. "\x") keeps the backslash literally. A trailing
    // backslash is consumed and yields an empty final field.
    static std::vector<std::string> SplitEscaped(const std::string& s);
    // Uppercase hex encoding of a byte vector.
    static std::string HexEncode(const std::vector<BYTE>& data);
    // Decodes a hex string back to bytes. Malformed input (invalid hex char
    // or odd length) returns whatever was decoded so far (possibly empty).
    static std::vector<BYTE> HexDecode(const std::string& hex);
};
