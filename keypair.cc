#include "pkcs11test.h"

using namespace std;  // So sue me

namespace pkcs11 {
namespace test {

class KeyPairTest : public ReadWriteSessionTest {
 public:
  KeyPairTest()
    : public_attrs_({CKA_ENCRYPT}),
      private_attrs_({CKA_DECRYPT}),
      keypair_(session_, public_attrs_, private_attrs_) {}
 protected:
  vector<CK_ATTRIBUTE_TYPE> public_attrs_;
  vector<CK_ATTRIBUTE_TYPE> private_attrs_;
  KeyPair keypair_;
};

TEST_F(KeyPairTest, EncryptDecrypt) {
  CK_BYTE plaintext[10];
  CK_ULONG plaintext_len = sizeof(plaintext);
  memcpy(plaintext, "0123456789", plaintext_len);
  // First encrypt the data with the public key.
  CK_MECHANISM mechanism = {CKM_RSA_PKCS, NULL_PTR, 0};
  CK_RV rv = g_fns->C_EncryptInit(session_, &mechanism, keypair_.public_handle());
  EXPECT_CKR_OK(rv);
  if (rv != CKR_OK) return;

  CK_BYTE ciphertext[1024];
  CK_ULONG ciphertext_len = sizeof(ciphertext);
  rv = g_fns->C_Encrypt(session_, plaintext, plaintext_len, ciphertext, &ciphertext_len);
  EXPECT_CKR_OK(rv);
  EXPECT_EQ(128, ciphertext_len);
  if (rv != CKR_OK) return;

  // Now decrypt the data with the private key.
  rv = g_fns->C_DecryptInit(session_, &mechanism, keypair_.private_handle());
  EXPECT_CKR_OK(rv);
  if (rv != CKR_OK) return;

  CK_BYTE recovered_plaintext[1024];
  CK_ULONG recovered_plaintext_len = sizeof(plaintext);
  rv = g_fns->C_Decrypt(session_, ciphertext, ciphertext_len, recovered_plaintext, &recovered_plaintext_len);
  EXPECT_CKR_OK(rv);
  EXPECT_EQ(plaintext_len, recovered_plaintext_len);
  EXPECT_EQ(0, memcmp(plaintext, recovered_plaintext, plaintext_len));
}

TEST_F(ReadWriteSessionTest, PublicExponent4Bytes) {
  CK_ULONG modulus_bits = 1024;
  CK_BYTE public_exponent_value[] = {0x00, 0x1, 0x0, 0x1}; // 65537=0x00010001
  vector<CK_ATTRIBUTE> public_attrs = {
    {CKA_ENCRYPT},
    {CKA_MODULUS_BITS, &modulus_bits, sizeof(modulus_bits)},
    {CKA_PUBLIC_EXPONENT, public_exponent_value, sizeof(public_exponent_value)},
  };
  vector<CK_ATTRIBUTE> private_attrs = {
    {CKA_DECRYPT},
  };
  CK_MECHANISM mechanism = {CKM_RSA_PKCS_KEY_PAIR_GEN, NULL_PTR, 0};
  CK_OBJECT_HANDLE public_key = INVALID_OBJECT_HANDLE;
  CK_OBJECT_HANDLE private_key = INVALID_OBJECT_HANDLE;
  EXPECT_CKR_OK(g_fns->C_GenerateKeyPair(session_, &mechanism,
                                         public_attrs.data(), public_attrs.size(),
                                         private_attrs.data(), private_attrs.size(),
                                         &public_key, &private_key));

  // Clean up
  if (public_key != INVALID_OBJECT_HANDLE) {
    EXPECT_CKR_OK(g_fns->C_DestroyObject(session_, public_key));
  }
  if (private_key != INVALID_OBJECT_HANDLE) {
    EXPECT_CKR_OK(g_fns->C_DestroyObject(session_, private_key));
  }
}

}  // namespace test
}  // namespace pkcs11