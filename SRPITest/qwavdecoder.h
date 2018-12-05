#ifndef QWAVDECODER_H
#define QWAVDECODER_H

#include <QObject>
#include <QByteArray>
#include <QAudioFormat>

class QWavDecoder : public QObject
{
    Q_OBJECT
public:
    explicit QWavDecoder(QObject *parent = nullptr);

public:
    /** Read a wav file to play audio into a buffer
    *
    * The header for a WAV file looks like this:
    * Positions	Sample Value	Description

    * The "RIFF" chunk descriptor
    * 0-3       "RIFF"	Marks the file as a riff file. Characters are each 1 byte long.
    * 4-7       Size of the overall file minus 8 bytes, in bytes.
    * 8-11      "WAVE"	File Type Header. For our purposes, it always equals "WAVE".
    *
    * The "fmt" sub-chhunk
    * 12-15     "fmt " format chunk marker. Includes trailing null
    * 16-19     length of the followed format chunck in bytes (16)
    * 20-21     type of format (1 is PCM)
    * 22-23     number of channels
    * 24-27     sample rate in Hertz.
    * 28-31     byte rate
    * 32-33     block align (NumChannels * BitsPerSample/8) in bytes
    * 34-35     bits per sample
    *
    * [The "FLLR" sub-chunk] - optional
    * 36-39     "FLLR" chunk marker
    * 40-43     Chunck size in bytes
    *
    * The "data" sub-chunk
    * 36-39     "data" chunk marker. Marks the beginning of the data section.
    * 40-43     File size (data)	Size of the data section.
    * 44...     Raw data
    *
    * Also for better understanding read this
    * - http://soundfile.sapp.org/doc/WaveFormat/
    * - https://stackoverflow.com/questions/13039846/what-do-the-bytes-in-a-wav-file-represent
    */
    static void readSoundRecord(const QString &_fileName, QAudioFormat &_format, QByteArray &_bytearray, bool _verbose);
};

#endif // QWAVDECODER_H
