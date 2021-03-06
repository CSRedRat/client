/*
 * This software is in the public domain, furnished "as is", without technical
 * support, and with no warranty, express or implied, as to its usefulness for
 * any purpose.
 *
 */

#pragma once

#include <QtTest>
#include <QDir>
#include <QString>

#include "transmissionchecksumvalidator.h"
#include "networkjobs.h"
#include "utility.h"
#include "filesystem.h"
#include "propagatorjobs.h"

using namespace OCC;

    class TestTransChecksumValidator : public QObject
    {
        Q_OBJECT

    private:
        QString _root;
        QString _testfile;
        QString _expectedError;
        QEventLoop     _loop;
        QByteArray     _expected;
        bool           _successDown;
        bool           _errorSeen;

    void processAndWait() {
            _loop.processEvents();
            Utility::usleep(200000);
            _loop.processEvents();
    }

    public slots:

    void slotUpValidated(const QByteArray& checksum) {
         qDebug() << "Checksum: " << checksum;
         QVERIFY(_expected == checksum );
    }

    void slotDownValidated() {
         _successDown = true;
    }

    void slotDownError( const QString& errMsg ) {
         QVERIFY(_expectedError == errMsg );
         _errorSeen = true;
    }

    private slots:

    void initTestCase() {
        qDebug() << Q_FUNC_INFO;
        _root = QDir::tempPath() + "/" + "test_" + QString::number(qrand());
        QDir rootDir(_root);

        rootDir.mkpath(_root );
        _testfile = _root+"/csFile";
        Utility::writeRandomFile( _testfile);

    }

    void testUploadChecksummingAdler() {

        TransmissionChecksumValidator *vali = new TransmissionChecksumValidator(_testfile, this);
        vali->setChecksumType("Adler32");

        connect(vali, SIGNAL(validated(QByteArray)), this, SLOT(slotUpValidated(QByteArray)));

        QString testfile = _testfile;
        _expected = "Adler32:"+FileSystem::calcAdler32( testfile );
        qDebug() << "XX Expected Checksum: " << _expected;
        vali->uploadValidation();

        usleep(5000);

        _loop.processEvents();
        delete vali;
    }

    void testUploadChecksummingMd5() {

        TransmissionChecksumValidator *vali = new TransmissionChecksumValidator(_testfile, this);
        vali->setChecksumType( OCC::checkSumMD5C );
        connect(vali, SIGNAL(validated(QByteArray)), this, SLOT(slotUpValidated(QByteArray)));

        _expected = checkSumMD5C;
        _expected.append(":"+FileSystem::calcMd5( _testfile ));
        vali->uploadValidation();

        usleep(2000);

        _loop.processEvents();
        delete vali;
    }

    void testUploadChecksummingSha1() {

        TransmissionChecksumValidator *vali = new TransmissionChecksumValidator(_testfile, this);
        vali->setChecksumType( OCC::checkSumSHA1C );
        connect(vali, SIGNAL(validated(QByteArray)), this, SLOT(slotUpValidated(QByteArray)));

        _expected = checkSumSHA1C;
        _expected.append(":"+FileSystem::calcSha1( _testfile ));

        vali->uploadValidation();

        usleep(2000);

        _loop.processEvents();
        delete vali;
    }

    void testDownloadChecksummingAdler() {

        QByteArray adler =  checkSumAdlerC;
        adler.append(":");
        adler.append(FileSystem::calcAdler32( _testfile ));
        _successDown = false;

        TransmissionChecksumValidator *vali = new TransmissionChecksumValidator(_testfile, this);
        vali->setChecksumType("Adler32");
        connect(vali, SIGNAL(validated(QByteArray)), this, SLOT(slotDownValidated()));
        connect(vali, SIGNAL(validationFailed(QString)), this, SLOT(slotDownError(QString)));
        vali->downloadValidation(adler);

        usleep(2000);

        _loop.processEvents();
        QVERIFY(_successDown);

        _expectedError = QLatin1String("The downloaded file does not match the checksum, it will be resumed.");
        _errorSeen = false;
        vali->downloadValidation("Adler32:543345");
        usleep(2000);
        _loop.processEvents();
        QVERIFY(_errorSeen);

        _expectedError = QLatin1String("The checksum header is malformed.");
        _errorSeen = false;
        vali->downloadValidation("Klaas32:543345");
        usleep(2000);
        _loop.processEvents();
        QVERIFY(_errorSeen);

        delete vali;
    }


    void cleanupTestCase() {
    }
};
