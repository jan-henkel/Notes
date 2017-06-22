#include "cryptointerface.h"

CryptoInterface::CryptoInterface(QObject *parent) : QObject(parent), pbkdf2iterations_(DEFAULT_PBKDF2_ITERATIONS), masterKeySet_(false)
{
    setEncryptionScheme(PBKDF2_SHA1_AES256);
}

bool CryptoInterface::verifyPasswordAndDecryptMasterKey(const CryptoPP::SecByteBlock &password, const CryptoPP::SecByteBlock &salt, const CryptoPP::SecByteBlock &iv, const CryptoPP::SecByteBlock &hk2, const CryptoPP::SecByteBlock &encryptedKey, CryptoPP::SecByteBlock &rK)
{
    CryptoPP::SecByteBlock k1,k2;
    //intermediate long key k=(k1,k2) from password and salt
    calcK(password,salt,k1,k2);
    //verify password, remove latter half of k if successful
    if(!verifyK2(hk2,k2))
        return false;
    //decrypt master key using k1
    decryptData(k1,iv,encryptedKey,rK);
    return true;
}

void CryptoInterface::calcK(const CryptoPP::SecByteBlock &password, const CryptoPP::SecByteBlock &salt, CryptoPP::SecByteBlock &rK1, CryptoPP::SecByteBlock &rK2)
{
    //derive long key K using pbkdf2(password,salt). second half of K is used as password verification, first half to encrypt / decrypt master key.
    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA1> keyDerivationAlgorithm;
    //default padding with 0x00 to allow for empty passwords
    CryptoPP::SecByteBlock k(64);//=keyDerivationAlgorithm.makeKey(password+CryptoPP::SecByteBlock(1,0),salt,64,pbkdf2iterations_); //length = 64 bytes = 512 bits
    CryptoPP::SecByteBlock zero(0x00,1);
    CryptoPP::SecByteBlock pw=password;
    pw+=zero;
    keyDerivationAlgorithm.DeriveKey(k.BytePtr(),64,0,pw.BytePtr(),pw.size(),salt,salt.size(),pbkdf2iterations_);
    rK1.resize(32);
    rK2.resize(32);
    for(int i=0;i<32;++i)
        {rK1[i]=k[i]; rK2[i]=k[i+32];}
}

bool CryptoInterface::verifyK2(const CryptoPP::SecByteBlock &hk2, CryptoPP::SecByteBlock &k2)
{
    CryptoPP::SHA1 hash;
    CryptoPP::SecByteBlock sha1val(20);
    hash.CalculateDigest(sha1val.BytePtr(),k2.BytePtr(),k2.size());
    if(hk2==sha1val)
        return true;
    else
        return false;
}

void CryptoInterface::decryptData(const CryptoPP::SecByteBlock &k1, const CryptoPP::SecByteBlock &iv, const CryptoPP::SecByteBlock &ciphertext, CryptoPP::SecByteBlock &rPlaintext)
{
    //aes-decrypt ciphertext using key k1, initialization vector iv. save to rPlaintext
    CryptoPP::CBC_Mode<AES256>::Decryption decrypt;
    rPlaintext.resize((ciphertext.SizeInBytes()/16-1)*16);
    decrypt.SetKeyWithIV(k1.BytePtr(),k1.SizeInBytes(),iv.BytePtr(),std::min((int)iv.SizeInBytes(),16));
    //for(int i=16;i<iv.SizeInBytes();++i)
    //    decrypt.ProcessByte(iv[i]);
    decrypt.ProcessData(rPlaintext.BytePtr(),ciphertext.BytePtr(),ciphertext.SizeInBytes());
}

void CryptoInterface::encryptData(const CryptoPP::SecByteBlock &k1, const CryptoPP::SecByteBlock &iv, CryptoPP::SecByteBlock &rCiphertext, const CryptoPP::SecByteBlock &plaintext)
{
    //aes-encrypt plaintext using key k1, initialization vector iv. save to rCiphertext
    CryptoPP::CBC_Mode<AES256>::Encryption encrypt;
    rCiphertext.resize((plaintext.SizeInBytes()/16+1)*16);
    encrypt.SetKeyWithIV(k1.BytePtr(),k1.SizeInBytes(),iv.BytePtr(),std::min((int)iv.SizeInBytes(),16));
    //for(int i=16;i<iv.SizeInBytes();++i)
    //    encrypt.ProcessByte(iv[i]);
    encrypt.ProcessData(rCiphertext.BytePtr(),plaintext.BytePtr(),plaintext.SizeInBytes());
}

CryptoInterface::Result CryptoInterface::readMasterKey(const CryptoPP::SecByteBlock &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    if(!file.exists())
        return NO_FILE;
    file.open(QFile::ReadOnly);

    //read encryption scheme
    file.read((char*)&this->encryptionScheme_,sizeof(EncryptionScheme));

    //read initialization vector (unencrypted)
    CryptoPP::SecByteBlock iv(16); //blocksize of AES is 128 bit regardless of keylength
    if(!file.read((char*)iv.BytePtr(),16))
        return WRONG_FILE_FORMAT;

    //read salt (unencrypted)
    CryptoPP::SecByteBlock salt(32);
    if(!file.read((char*)salt.BytePtr(),32))
        return WRONG_FILE_FORMAT;

    //read sha1 hash of latter half k2 of derived intermediate key
    //to be used for password verification
    CryptoPP::SecByteBlock hk2(20);
    if(!file.read((char*)hk2.BytePtr(),20))
        return WRONG_FILE_FORMAT;

    //read encrypted master key. 48 bytes due to padding.
    CryptoPP::SecByteBlock encryptedKey(48);
    if(!file.read((char*)encryptedKey.BytePtr(),48))
        return WRONG_FILE_FORMAT;
    if(!file.atEnd())
        return WRONG_FILE_FORMAT;
    file.close();

    //generate long intermediate key k=(k1,k2) from password, using salt
    CryptoPP::SecByteBlock k1;
    CryptoPP::SecByteBlock k2;
    calcK(password,salt,k1,k2);

    //verify password by comparing the second half of k to k2
    if(!verifyK2(hk2,k2))
        return WRONG_PASSWORD;
    //if password is correct, decrypt the master key
    decryptData(k1,iv,encryptedKey,masterkey_);
    masterKeySet_=true;
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::saveMasterKey(const CryptoPP::SecByteBlock &password, QString fileName)
{
    //open file, return error on failure
    QFile file(fileName);
    file.open(QFile::WriteOnly);

    //write encryption scheme
    file.write((char*)&this->encryptionScheme_,sizeof(EncryptionScheme));

    //write random iv (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    CryptoPP::SecByteBlock iv(16);
    CryptoInterface::randomize(iv);
    file.write((const char*)iv.BytePtr(),16);

    //write random salt (doesn't require lots of entropy since it remains public and doesn't need to be guessed by an attacker)
    CryptoPP::SecByteBlock salt(32);
    CryptoInterface::randomize(salt);
    file.write((const char*)salt.BytePtr(),32);

    //derive long intermediate key k using password and salt
    CryptoPP::SecByteBlock k1;
    CryptoPP::SecByteBlock k2;
    calcK(password,salt,k1,k2);

    //write latter half of k to file unencrypted
    CryptoPP::SHA1 hash;
    CryptoPP::SecByteBlock digest(hash.DigestSize());
    hash.CalculateDigest(digest.BytePtr(),k2.BytePtr(),k2.SizeInBytes());
    file.write((const char*)digest.BytePtr(),digest.SizeInBytes());

    //encrypt master key with first half of k and initialization vector
    CryptoPP::SecByteBlock encryptedKey;
    encryptData(k1,iv,encryptedKey,masterkey_);
    file.write((const char*)encryptedKey.BytePtr(),48);
    file.close();
    return SUCCESS;
}

void CryptoInterface::setMasterKey(const CryptoPP::SecByteBlock &key)
{
    masterkey_=key;
    masterKeySet_=true;
}

void CryptoInterface::setRandomMasterKey()
{
    masterkey_.resize(32);
    CryptoInterface::randomize(masterkey_);
    masterKeySet_=true;
}

CryptoInterface::Result CryptoInterface::encrypt(CryptoPP::SecByteBlock &rCipher,const CryptoPP::SecByteBlock &plain,const CryptoPP::SecByteBlock &iv)
{
    if(!masterKeySet_)
        return KEY_NOT_SET;
    encryptData(masterkey_,iv,rCipher,plain);
    return SUCCESS;
}

CryptoInterface::Result CryptoInterface::decrypt(const CryptoPP::SecByteBlock &cipher, CryptoPP::SecByteBlock &rPlain, const CryptoPP::SecByteBlock &iv)
{
    if(!masterKeySet_)
        return KEY_NOT_SET;
    decryptData(masterkey_,iv,cipher,rPlain);
    return SUCCESS;
}

CryptoPP::SecByteBlock CryptoInterface::encrypt(const CryptoPP::SecByteBlock &plain, const CryptoPP::SecByteBlock &iv)
{
    CryptoPP::SecByteBlock ret;
    encrypt(ret,plain,iv);
    return ret;
}

CryptoPP::SecByteBlock CryptoInterface::decrypt(const CryptoPP::SecByteBlock &cipher, const CryptoPP::SecByteBlock &iv)
{
    CryptoPP::SecByteBlock ret;
    decrypt(cipher,ret,iv);
    return ret;
}

void CryptoInterface::unsetMasterKey()
{
    //discard previous master key
    masterkey_.CleanNew(32);
    masterKeySet_=false;
}
