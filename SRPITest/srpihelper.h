#ifndef IRPIHELPER_H
#define IRPIHELPER_H

#include <cmath>
#include <cstring>
#include <iostream>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QImage>
#include <QDir>

#ifndef USE_CUSTOM_WAV_DECODER
#include <QEventLoop>
#include <QAudioFormat>
#include <QAudioDecoder>
#include <QCoreApplication>
#else
#include "qwavdecoder.h"
#endif

#include "srpi.h"

inline std::ostream&
operator<<(
    std::ostream &s,
    const QImage::Format &_format)
{
    switch (_format) {
        case QImage::Format_ARGB32:
            return (s << "Format_ARGB32");
        case QImage::Format_RGB32:
            return (s << "Format_RGB32");
        case QImage::Format_RGB888:
            return (s << "Format_RGB888");
        case QImage::Format_Grayscale8:
            return (s << "Format_Grayscale8");
        default:
            return (s << "Undefined");
    }
}

inline std::ostream&
operator<<(
    std::ostream &s,
    const QString &_qstring)
{
    return s << _qstring.toLocal8Bit().constData();
}

SRPI::SoundRecord readSoundRecord(const QString &_filename, bool _verbose=false)
{
    QAudioFormat _format;
    QByteArray _bytearray;

#ifndef USE_CUSTOM_WAV_DECODER

    // Target audio format
    QAudioFormat _tf;
    _tf.setCodec("audio/pcm");
    _tf.setByteOrder(QAudioFormat::LittleEndian);
    _tf.setSampleType(QAudioFormat::SignedInt);
    _tf.setSampleSize(16);

    QAudioDecoder _audiodecoder;
    if(_verbose)
        QObject::connect(&_audiodecoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),[&_audiodecoder](QAudioDecoder::Error _error) {
                            Q_UNUSED(_error);
                            std::cout << _audiodecoder.errorString() << std::endl;
                        });
    _audiodecoder.setAudioFormat(_tf);
    _audiodecoder.setSourceFilename(_filename);

    QEventLoop _el;
    QObject::connect(&_audiodecoder, SIGNAL(finished()), &_el, SLOT(quit()));
    QObject::connect(&_audiodecoder, SIGNAL(error(QAudioDecoder::Error)), &_el, SLOT(quit()));
    QObject::connect(&_audiodecoder, &QAudioDecoder::bufferReady, [&_audiodecoder, &_bytearray, &_format] () {
                QAudioBuffer _ab = _audiodecoder.read();
                _format = _ab.format();
                _bytearray.append(QByteArray(_ab.constData<char>(),_ab.byteCount()));
            });
    _audiodecoder.start();
    _el.exec();
    if(_verbose)
        std::cout << "\tRecord size (bytes): " << _bytearray.size() << std::endl;
#else
    QWavDecoder::readSoundRecord(_filename,_format,_bytearray,_verbose);
#endif

    if((_format.sampleSize() % 8) != 0) {
        std::cout << "Unsupported sample size (" << _format.sampleSize() << ")!" << std::endl;
        return SRPI::SoundRecord();
    }
    if(_format.byteOrder() != QAudioFormat::LittleEndian) {
        std::cout << "Unsupported byte order (" << _format.byteOrder() << ")!" << std::endl;
        return SRPI::SoundRecord();
    }
    if(_format.sampleType() != QAudioFormat::SignedInt) {
        std::cout << "Unsupported sample type " << _format.sampleType() << ")!" << std::endl;
        return SRPI::SoundRecord();
    }

    std::shared_ptr<uint8_t>_sharedptr(new uint8_t[_bytearray.size()], std::default_delete<uint8_t[]>());
    std::memcpy(_sharedptr.get(), _bytearray.constData(), static_cast<size_t>(_bytearray.size()));
    return SRPI::SoundRecord(static_cast<uint32_t>(_bytearray.size() / _format.bytesPerFrame()),
                             static_cast<uint8_t>(_format.channelCount()),
                             static_cast<uint8_t>(_format.sampleSize()),
                             _sharedptr);
}

//---------------------------------------------------
void computeFARandFRR(const std::vector<std::vector<SRPI::Candidate>> &_vcandidates, const std::vector<bool> &_vdecisions, const std::vector<size_t> &_vtruelabel, double &_far, double &_frr)
{
    size_t _tp = 0, _tn = 0, _fn = 0, _fp = 0;
    for(size_t i = 0; i < _vcandidates.size(); ++i) {
        // as irpi.h says - most similar entries appear first
        const SRPI::Candidate &_mate = _vcandidates[i][0];
        if(_vdecisions[i] == true) { // Vendor reports that mate has been found
            if(_mate.label == _vtruelabel[i])
                _tp++;
            else
                _fp++;
        } else { // Vendor reports that mate can not be found
            if(_mate.label == _vtruelabel[i])
                _fn++;
            else
                _tn++;
        }
    }
    _far = static_cast<double>(_fp) / (_fp + _tp + 1.e-6);
    _frr = static_cast<double>(_fn) / (_tn + _fn + 1.e-6);
}
//--------------------------------------------------

struct CMCPoint
{
    CMCPoint() : mTPIR(0), rank(0) {}
    double mTPIR; // aka probability of true positive in top rank
    size_t  rank;
};

std::vector<CMCPoint> computeCMC(const std::vector<std::vector<SRPI::Candidate>> &_vcandidates, const std::vector<size_t> &_vtruelabels, const size_t _enrolllabelmax)
{
    // We need to count only assigned elements
    size_t length = 0;
    const std::vector<SRPI::Candidate> &_candidates = _vcandidates[0];
    for(size_t &i = length; i < _candidates.size(); ++i) {
        if(_candidates[i].isAssigned == false)
            break;
    }
    // Let's count frequencies for ranks
    std::vector<size_t> _vrankfrequency(length,0);
    size_t _instances = 0;
    for(size_t i = 0; i < _vcandidates.size(); ++i) {
        if(_vtruelabels[i] <= _enrolllabelmax) { // need to count only instances with mates
            _instances++;
            const std::vector<SRPI::Candidate> &_candidates = _vcandidates[i];
            for(size_t j = 0; j < length; ++j) {
                if(_candidates[j].label == _vtruelabels[i]) {
                    _vrankfrequency[j]++;
                    break;
                }
            }
        }
    }
    // We are ready to save points
    std::vector<CMCPoint> _vCMC(length,CMCPoint());
    for(size_t i = 0; i < length; ++i) {
        _vCMC[i].rank = i + 1;
        for(size_t j = 0; j < _vCMC[i].rank; ++j) {
            _vCMC[i].mTPIR += _vrankfrequency[j];
        }
        _vCMC[i].mTPIR /= _instances + 1e-6; // add epsilon here to prevent nan when _instances == 0
    }
    return _vCMC;
}

QJsonArray serializeCMC(const std::vector<CMCPoint> &_cmc)
{
    QJsonArray _jsonarr;
    for(size_t i = 0; i < _cmc.size(); ++i) {
        QJsonObject _jsonobj;
        _jsonobj["Rank"] = static_cast<qint64>(_cmc[i].rank);
        _jsonobj["TPIR"] = _cmc[i].mTPIR;
        _jsonarr.push_back(qMove(_jsonobj));
    }
    return _jsonarr;
}

//--------------------------------------------------
void showTimeConsumption(qint64 secondstotal)
{
    qint64 days    = secondstotal / 86400;
    qint64 hours   = (secondstotal - days * 86400) / 3600;
    qint64 minutes = (secondstotal - days * 86400 - hours * 3600) / 60;
    qint64 seconds = secondstotal - days * 86400 - hours * 3600 - minutes * 60;
    std::cout << std::endl << "Test has been complited successfully" << std::endl
              << " It took: " << days << " days "
              << hours << " hours "
              << minutes << " minutes and "
              << seconds << " seconds" << std::endl;
}

#endif // IRPIHELPER_H
