#ifndef TEST_PERSONAFILE_H
#define TEST_PERSONAFILE_H

#include <QObject>

class TestPersonaFile : public QObject
{
    Q_OBJECT

private slots:
    void parsesEnglishFrontmatter();
    void parsesChineseKeys();
    void withoutFrontmatterUsesFilename();
    void frontmatterWithoutNameUsesFilename();
    void missingFileFails();
    void malformedFrontmatterFails();
    void rendersRoundTrip();
    void sanitizesFileIds();
    void asyncParseMatchesSync();
};

#endif // TEST_PERSONAFILE_H
