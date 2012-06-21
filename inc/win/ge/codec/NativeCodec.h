// NativeCodec.h

#ifndef NATIVE_CODEC_H
#define NATIVE_CODEC_H

#include <ge/common.h>
#include <ge/codec/Codec.h>

/*
 * Conversion
 */
class NativeCodec : public Codec
{
public:
    NativeCodec();
    NativeCodec(const NativeCodec& other);
    NativeCodec& operator=(const NativeCodec& other);

    /*
     * Initialize to the encoding with the passed name. Returns true if the
     * encoding is supported by the underlying OS library.
     */
    bool init(const StringRef& encodingName);

    bool encodingToUtf8(const char* source, size_t sourceLen,
                        List<char>* dest) OVERRIDE;

    bool utf8ToEncoding(const char* source, size_t sourceLen,
                        List<char>* dest) OVERRIDE;

private:
    uint32 m_codePage;
};

#endif // NATIVE_CODEC_H
