/*
* Copyright (c) 2003-2013 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __PWSFILEV4_H
#define __PWSFILEV4_H

// PWSfileV4.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------


#include "PWSfile.h"
#include "TwoFish.h"
#include "sha256.h"
#include "hmac.h"
#include "UTF8Conv.h"

#include <vector>

class PWSfileV4 : public PWSfile
{
public:
  enum Cipher {PWTwoFish, PWAES};
  
  static int CheckPasskey(const StringX &filename,
                          const StringX &passkey,
                          FILE *a_fd = NULL,
                          unsigned char *aPtag = NULL, uint32 *nIter = NULL);
  static bool IsV4x(const StringX &filename, VERSION &v);

  PWSfileV4(const StringX &filename, RWmode mode, VERSION version);
  ~PWSfileV4();

  virtual int Open(const StringX &passkey);
  virtual int Close();

  virtual int WriteRecord(const CItemData &item);
  virtual int ReadRecord(CItemData &item);

  uint32 GetNHashIters() const; // for current keyblock
  void SetNHashIters(uint32 N); // for current keyblock
  
  // Following for low-level details that changed between format versions
  virtual size_t timeFieldLen() const {return 5;} // Experimental

 private:
  // Format constants:
  enum {PWSaltLength = 32, KLEN = 32, KWLEN = (KLEN + 8), NONCELEN = 32};
  struct KeyBlock { // See formatV4.txt
  KeyBlock() : m_nHashIters(MIN_HASH_ITERATIONS) {}
    unsigned char m_salt[PWSaltLength];
    uint32 m_nHashIters;
    unsigned char m_kw_k[KWLEN];
    unsigned char m_kw_l[KWLEN];
  };
  const size_t KBLEN = PWSaltLength + sizeof(uint32) + KWLEN + KWLEN;
  std::vector<KeyBlock> m_keyblocks;
  unsigned m_current_keyblock; // index
  unsigned char m_nonce[NONCELEN]; // 256 bit nonce
  ulong64 m_effectiveFileLength; // for read = fileLength - |HMAC|
  Cipher m_cipher;
  friend struct KeyBlockWriter;
  unsigned char m_ipthing[TwoFish::BLOCKSIZE]; // for CBC
  unsigned char m_key[KLEN]; // K
  unsigned char m_ell[KLEN]; // L
  HMAC<SHA256, SHA256::HASHLEN, SHA256::BLOCKSIZE> m_hmac; // L
  CUTF8Conv m_utf8conv;
  int ParseKeyBlocks(const StringX &passkey);
  int ReadKeyBlock(); // can return SUCCESS or END_OF_FILE
  int TryKeyBlock(unsigned index, const StringX &passkey,
                  unsigned char K[KLEN], unsigned char L[KLEN]);
  void ComputeEndKB(const unsigned char hnonce[SHA256::HASHLEN],
                    unsigned char digest[SHA256::HASHLEN]);
  bool EndKeyBlocks(const unsigned char calc_hnonce[SHA256::HASHLEN]);
  bool VerifyKeyBlocks();
  virtual size_t WriteCBC(unsigned char type, const StringX &data);
  virtual size_t WriteCBC(unsigned char type, const unsigned char *data,
                          size_t length);

  virtual size_t ReadCBC(unsigned char &type, unsigned char* &data,
                         size_t &length);

  void SetupKeyBlocksForWrite();
  bool WriteKeyBlocks();
  int WriteHeader();
  int ReadHeader();

  static int SanityCheck(FILE *stream); // Check for TAG and EOF marker
  static void StretchKey(const unsigned char *salt, unsigned long saltLen,
                         const StringX &passkey, uint32 N,
                         unsigned char *Ptag, unsigned long PtagLen);
};
#endif /* __PWSFILEV4_H */
