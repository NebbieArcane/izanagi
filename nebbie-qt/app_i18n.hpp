#pragma once

#include <QString>

namespace nebbie::qt {

enum class AppLanguage {
    Italian,
    English,
};

AppLanguage currentAppLanguage();
void setAppLanguage(AppLanguage language);

AppLanguage parseLanguageCode(const QString& code);
QString languageCode(AppLanguage language);

QString appTr(const char* key);
QString appTr(const char* key, const QString& arg1);
QString appTr(const char* key, const QString& arg1, const QString& arg2);
QString appTr(const char* key, const QString& arg1, const QString& arg2, const QString& arg3);

QString izanagiDisplayName();
QString cypherDisplayName();
QString izanagiWindowTitle();
QString cypherWindowTitle();
QString izanagiAboutText(const QString& version);
QString cypherAboutText(const QString& version);

QString githubReleaseRepo();

} // namespace nebbie::qt
