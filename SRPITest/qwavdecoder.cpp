#include "qwavdecoder.h"

#include <iostream>

#include <QFile>
#include <QDataStream>

QWavDecoder::QWavDecoder(QObject *parent) : QObject(parent)
{

}

void QWavDecoder::readSoundRecord(const QString &_fileName, QAudioFormat &_format, QByteArray &_bytearray, bool _verbose)
{
    if(QString(_fileName).section(".",1) != "wav") {
        std::cout << "Unsupported file format, 'wav' only allowed!" << std::endl;
        return;
    }

    QFile _file(_fileName);
    if(_file.open(QFile::ReadOnly) == false) {
        std::cout << "Can not open file!" << std::endl;
        return;
    }

    // Define the header components
    char fileType[4];
    qint32 fileSize;
    char waveName[4];
    char fmtName[4];
    qint32 fmtLength;
    qint16 fmtType;
    qint16 numberOfChannels;
    qint32 sampleRate;
    qint32 byteRate;
    qint16 frameSize;
    qint16 bitsPerSample;
    char dataHeader[4];
    qint32 dataSize;

    // Read in the whole thing
    QByteArray _fileContent = _file.readAll();
    QDataStream _ds(&_fileContent,QIODevice::ReadOnly);
    _ds.setByteOrder(QDataStream::LittleEndian);

    // Chunk
    _ds.readRawData(fileType,4); // "RIFF"
    _ds >> fileSize; // File Size
    _ds.readRawData(waveName,4); // "WAVE"

    // First sub-chunk
    _ds.readRawData(fmtName,4); // "fmt"
    _ds >> fmtLength; // Format length
    /*if(fmtLength != 16) {
        qWarning(tr("QWavDecoder: unsupported 'wav' header!").toUtf8().constData());
        return;
    }*/
    _ds >> fmtType; // Format type
    _format.setCodec(fmtType == 1 ? "audio/pcm" : QString::number(fmtType));
    _ds >> numberOfChannels; // Number of channels
    _format.setChannelCount(numberOfChannels);
    _ds >> sampleRate; // Sample rate
    _format.setSampleRate(sampleRate);
    _ds >> byteRate; // (Sample Rate * BitsPerSample * Channels) / 8
    _ds >> frameSize; // (BitsPerSample * Channels) / 8
    _ds >> bitsPerSample; // Bits per sample
    _format.setSampleSize(bitsPerSample);

    // Second sub-chunk may be optional "FLLR" sub-chunk
    _ds.readRawData(dataHeader,4); // "data" header
    _ds >> dataSize; // Data Size
    if(QString::fromUtf8(dataHeader).contains("FLLR")) {
        QByteArray _fllrdata;
        _fllrdata.resize(dataSize);
        _ds.readRawData(_fllrdata.data(),_fllrdata.size());
        _ds.readRawData(dataHeader,4); // "data" header
        _ds >> dataSize; // Data Size
    }

    // Print the header
    if(_verbose)
        std::cout   << "\tWAV file header content" << std::endl
                    << "\tFile Type: " << std::string(&fileType[0],4) << std::endl
                    << "\tFile Size: " << fileSize << std::endl
                    << "\tWAV Marker: " << std::string(&waveName[0],4) << std::endl
                    << "\tFormat Name: " << std::string(&fmtName[0],4) << std::endl
                    << "\tFormat Length: " << fmtLength << std::endl
                    << "\tFormat Type: " << fmtType << std::endl
                    << "\tNumber of Channels: " << numberOfChannels << std::endl
                    << "\tSample Rate: " << sampleRate << std::endl
                    << "\tByte Rate: " << byteRate << std::endl
                    << "\tBlock Align: " << frameSize << std::endl
                    << "\tBits per Sample: " << bitsPerSample << std::endl
                    << "\tData Header: " << std::string(&dataHeader[0],4) << std::endl
                    << "\tData Size: " << dataSize << std::endl;

    // Now pull out the data
    _bytearray.resize(dataSize);
    _ds.readRawData(_bytearray.data(),_bytearray.size());
    _format.setByteOrder(QAudioFormat::LittleEndian);
    _format.setSampleType(QAudioFormat::SignedInt);
}
