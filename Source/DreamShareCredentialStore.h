#pragma once

#include <juce_core/juce_core.h>

#if JUCE_WINDOWS
#include <windows.h>
#include <wincred.h>
#endif

namespace dm {

struct DreamShareCredential {
    juce::String user;
    juce::String token;
};

inline juce::String credentialTargetForUser(const juce::String& user) {
    auto safeUser = user.trim().toLowerCase();
    safeUser = safeUser.replaceCharacters(juce::String("\\/:*?\"<>|"), juce::String("_________"));
    return "Dreamdaw/DreamMasterLite/" + safeUser + "/" + juce::String::toHexString(user.hashCode64());
}

inline juce::PropertiesFile::Options dreamSharePreferenceOptions() {
    juce::PropertiesFile::Options options;
    options.applicationName = "DreamMasterLite";
    options.folderName = "Dreamdaw";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    return options;
}

inline juce::PropertiesFile dreamSharePreferences() {
    return juce::PropertiesFile(dreamSharePreferenceOptions());
}

inline DreamShareCredential readDreamShareCredential() {
#if JUCE_WINDOWS
    struct CredentialGuard {
        PCREDENTIALW value = nullptr;
        ~CredentialGuard() { if (value != nullptr) CredFree(value); }
    } guard;

    auto preferences = dreamSharePreferences();
    const auto user = preferences.getValue("dreamShareCredentialUser");
    if (user.isEmpty()
        || !CredReadW(credentialTargetForUser(user).toWideCharPointer(), CRED_TYPE_GENERIC, 0, &guard.value)
        || guard.value == nullptr)
        return {};
    const auto* credential = guard.value;
    return {credential->UserName == nullptr ? juce::String() : juce::String(credential->UserName),
            juce::String::fromUTF8(reinterpret_cast<const char*>(credential->CredentialBlob),
                                   static_cast<int>(credential->CredentialBlobSize))};
#else
    return {};
#endif
}

inline bool writeDreamShareCredential(const DreamShareCredential& credential) {
#if JUCE_WINDOWS
    if (credential.user.isEmpty() || credential.token.isEmpty())
        return false;
    const auto target = credentialTargetForUser(credential.user);
    const auto user = credential.user.toWideCharPointer();
    const auto token = credential.token.toUTF8();
    CREDENTIALW entry{};
    entry.Type = CRED_TYPE_GENERIC;
    entry.TargetName = const_cast<LPWSTR>(target.toWideCharPointer());
    entry.UserName = const_cast<LPWSTR>(user);
    entry.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(token.getAddress()));
    entry.CredentialBlobSize = static_cast<DWORD>(token.sizeInBytes() - 1);
    entry.Persist = CRED_PERSIST_LOCAL_MACHINE;
    if (CredWriteW(&entry, 0) == FALSE)
        return false;
    auto preferences = dreamSharePreferences();
    const auto previousUser = preferences.getValue("dreamShareCredentialUser");
    preferences.setValue("dreamShareCredentialUser", credential.user);
    if (!preferences.saveIfNeeded()) {
        CredDeleteW(target.toWideCharPointer(), CRED_TYPE_GENERIC, 0);
        return false;
    }
    if (previousUser.isNotEmpty() && previousUser != credential.user)
        CredDeleteW(credentialTargetForUser(previousUser).toWideCharPointer(), CRED_TYPE_GENERIC, 0);
    return true;
#else
    juce::ignoreUnused(credential);
    return false;
#endif
}

inline void deleteDreamShareCredential(const juce::String& user) {
#if JUCE_WINDOWS
    if (user.isNotEmpty()) {
        CredDeleteW(credentialTargetForUser(user).toWideCharPointer(), CRED_TYPE_GENERIC, 0);
        auto preferences = dreamSharePreferences();
        if (preferences.getValue("dreamShareCredentialUser") == user) {
            preferences.removeValue("dreamShareCredentialUser");
            preferences.saveIfNeeded();
        }
    }
#else
    juce::ignoreUnused(user);
#endif
}

} // namespace dm
