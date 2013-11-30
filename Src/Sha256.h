/* Sha256.h -- SHA-256 Hash
2013-11-27 : Unknown : Public domain
2010-06-11 : Igor Pavlov : Public domain */

#pragma once

class Sha256
{
public:

  static const unsigned int blockSize = 64;

  Sha256() {reset();}

  void reset();

  void update(const unsigned char* data, unsigned int size);
  void finalize(unsigned char *digest);

  static QByteArray hash(const QByteArray& data)
  {
    Sha256 sha256;
    sha256.update((const unsigned char*)data.data(), data.length());
    QByteArray result(32, 0);
    sha256.finalize((unsigned char*)(char*)result.data());
    return result;
  }

  static QByteArray hmac(const QByteArray& key, const QByteArray& message)
  {
    QByteArray hashKey;
    if(key.length() > blockSize)
      hashKey = hash(key);
    else
      hashKey = key;
    hashKey += QByteArray(blockSize - hashKey.length(), 0);
    
    QByteArray oKeyPad = hashKey;
    QByteArray iKeyPad = hashKey;
    unsigned char* oKeyPadBuf = (unsigned char*)(char*)oKeyPad.data();
    unsigned char* iKeyPadBuf = (unsigned char*)(char*)iKeyPad.data();
    for(int i = 0; i < 64; ++i)
    {
      iKeyPadBuf[i] ^= 0x36;
      oKeyPadBuf[i] ^= 0x5c;
    }
    return hash(oKeyPad + hash(iKeyPad + message));
  }

public: // TODO: private
  unsigned int state[8];
  unsigned __int64 count;
  unsigned char buffer[64];
};
