#include "cryptobuffer.h"

CryptoBuffer::CryptoBuffer(QObject *parent) : QObject(parent), keyIVSet_(false)
{
    QCA::init();
}

bool CryptoBuffer::calcKey(const QCA::SecureArray &password, const QCA::SecureArray &salt, const QCA::SecureArray &iv, const QCA::SecureArray &k2, const QCA::SecureArray &encryptedKey, QCA::SecureArray &rKey)
{
    QCA::SymmetricKey k;
    calcK(password,salt,k);
    if(!verifyK2AndTruncateK(k2,k))
        return false;
    decryptData(k,iv,encryptedKey,rKey);
    return true;
}

void CryptoBuffer::calcK(const QCA::SecureArray &password, const QCA::SecureArray &salt, QCA::SecureArray &rK)
{
    QCA::PBKDF2 keyDerivationAlgorithm("sha512");
    rK=keyDerivationAlgorithm.makeKey(password,salt,64,10000);
}

bool CryptoBuffer::verifyK2AndTruncateK(const QCA::SecureArray &k2, QCA::SecureArray &k)
{
    for(int i=32;i<64;++i)
    {
        if(k[i]!=k2[i-32])
            return false;
    }
    k.resize(32);
    return true;
}

void CryptoBuffer::decryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, const QCA::SecureArray &ciphertext, QCA::SecureArray &rPlaintext)
{
    QCA::Cipher cipher("aes256",QCA::Cipher::CBC,QCA::Cipher::DefaultPadding,QCA::Decode,k1,iv);
    cipher.update(ciphertext);
    rPlaintext=QCA::SecureArray(cipher.final());
}

void CryptoBuffer::encryptData(const QCA::SecureArray &k1, const QCA::SecureArray &iv, QCA::SecureArray &rCiphertext, const QCA::SecureArray &plaintext)
{
    QCA::Cipher cipher("aes256",QCA::Cipher::CBC,QCA::Cipher::DefaultPadding,QCA::Encode,k1,iv);
    cipher.update(plaintext);
    rCiphertext=QCA::SecureArray(cipher.final());
}

CryptoBuffer::Result CryptoBuffer::readKeyIV(const QCA::SecureArray &password, QString fileName)
{
    QFile file(fileName);
    if(!file.exists())
        return NO_FILE;
    file.open(QFile::ReadOnly);
    QCA::SecureArray iv(32);
    if(!file.read(iv.data(),32))
        return WRONG_FILE_FORMAT;
    QCA::SecureArray salt(32);
    if(!file.read(salt.data(),32))
        return WRONG_FILE_FORMAT;
    QCA::SecureArray k2(32);
    if(!file.read(k2.data(),32))
        return WRONG_FILE_FORMAT;
    QCA::SecureArray encryptedKey(32);
    if(!file.read(encryptedKey.data(),32))
        return WRONG_FILE_FORMAT;
    QCA::SecureArray encryptedIV(32);
    if(!file.read(encryptedIV.data(),32))
        return WRONG_FILE_FORMAT;
    if(!file.atEnd())
        return WRONG_FILE_FORMAT;
    file.close();

    QCA::SymmetricKey k;
    //generate key from password, using salt
    calcK(password,salt,k);
    //verify password by comparing the second half of k to k2
    if(!verifyK2AndTruncateK(k2,k))
        return WRONG_PASSWORD;
    //if password is correct, decrypt the main key and initialization vector
    decryptData(k,iv,encryptedKey,key_);
    decryptData(k,iv,encryptedIV,iv_);
    keyIVSet_=true;
    return SUCCESS;
}

CryptoBuffer::Result CryptoBuffer::writeKeyIV(const QCA::SecureArray &password, QString fileName)
{
    QFile file(fileName);
    file.open(QFile::WriteOnly);
    QCA::InitializationVector iv(32);
    file.write(iv.data(),32);
    QCA::InitializationVector salt(32);
    file.write(salt.data(),32);
    QCA::SymmetricKey k;
    calcK(password,salt,k);
    file.write(&k[32],32);
    k.resize(32);
    QCA::SymmetricKey encryptedKey(32);
    encryptData(k,iv,encryptedKey,key_);
    file.write(encryptedKey.data(),32);
    QCA::SecureArray encryptedIV(32);
    encryptData(k,iv,encryptedIV,iv_);
    file.write(encryptedIV.data(),32);
    file.close();
    return SUCCESS;
}

void CryptoBuffer::setKeyIV(const QCA::SymmetricKey &key, const QCA::SymmetricKey &iv)
{
    key_=key;
    iv_=iv;
    keyIVSet_=true;
}

void CryptoBuffer::setRandomKeyIV()
{
    key_=QCA::InitializationVector(32);
    iv_=QCA::InitializationVector(32);
    keyIVSet_=true;
}

CryptoBuffer::Result CryptoBuffer::encrypt(QCA::SecureArray &rCipher,const QCA::SecureArray &plain,const QCA::InitializationVector &iv)
{
    if(!keyIVSet_)
        return KEY_NOT_SET;
    encryptData(key_,iv,rCipher,plain);
    return SUCCESS;
}

CryptoBuffer::Result CryptoBuffer::decrypt(const QCA::SecureArray &cipher, QCA::SecureArray &rPlain,const QCA::InitializationVector &iv)
{
    if(!keyIVSet_)
        return KEY_NOT_SET;
    decryptData(key_,iv,cipher,rPlain);
    return SUCCESS;
}

QCA::SecureArray CryptoBuffer::encrypt(const QCA::SecureArray &plain, const QCA::InitializationVector &iv)
{
    QCA::SecureArray ret;
    encrypt(ret,plain,iv);
    return ret;
}

QCA::SecureArray CryptoBuffer::encrypt(const QCA::SecureArray &plain)
{
    return encrypt(plain,iv_);
}

QCA::SecureArray CryptoBuffer::decrypt(const QCA::SecureArray &cipher, const QCA::InitializationVector &iv)
{
    QCA::SecureArray ret;
    decrypt(cipher,ret,iv);
    return ret;
}

QCA::SecureArray CryptoBuffer::decrypt(const QCA::SecureArray &cipher)
{
    return decrypt(cipher,iv_);
}

CryptoBuffer::Result CryptoBuffer::encrypt(QCA::SecureArray &rCipher,const QCA::SecureArray &plain)
{
    return encrypt(rCipher,plain,iv_);
}

CryptoBuffer::Result CryptoBuffer::decrypt(const QCA::SecureArray &cipher, QCA::SecureArray &rPlain)
{
    return decrypt(cipher,rPlain,iv_);
}

void CryptoBuffer::scrambleMemory(QCA::SecureArray &memory, quint64 size)
{
    memory.fill(char(0),size-1);
    for(unsigned int i=0;i<size;++i)
        memory[i]=QCA::Random::randomChar();
}
