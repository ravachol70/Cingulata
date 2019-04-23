#include <tfhe_bit_exec.hxx>

#include <tfhe.h>
#include <tfhe_io.h>

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace std;
using namespace cingulata;

class TfheBitExec::TfheContext {
public:
  TfheContext(const std::string& p_filename, const KeyType p_keytype) {
    ifstream file(p_filename.c_str());
    if(not file.is_open()) {
      cerr << "ERROR TfheContext: Cannot open key file '" << p_filename << "'" << endl;
      abort();
    }

    if (p_keytype == Secret) {
      m_sk = new_tfheGateBootstrappingSecretKeySet_fromStream(file);
      m_pk = const_cast<TFheGateBootstrappingCloudKeySet*>(&(m_sk->cloud)); //no other easy way to avoid const cast
    } else {
      m_sk = nullptr;
      m_pk = new_tfheGateBootstrappingCloudKeySet_fromStream(file);
    }
    file.close();

    m_params = m_pk->params;
  }

  ~TfheContext() {
    if (m_sk != nullptr) {
      delete_gate_bootstrapping_secret_keyset(m_sk);
    } else {
      delete_gate_bootstrapping_cloud_keyset(m_pk);
    }
  }

  const TFheGateBootstrappingSecretKeySet *const sk() const {
    assert(m_sk != nullptr && "secret key was not set");
    return m_sk;
  }

  const TFheGateBootstrappingCloudKeySet *const pk() const {
    return m_pk;
  }

  const TFheGateBootstrappingParameterSet *const params() const {
    return m_params;
  }

private:
  TFheGateBootstrappingSecretKeySet* m_sk = nullptr;
  TFheGateBootstrappingCloudKeySet* m_pk = nullptr;
  const TFheGateBootstrappingParameterSet* m_params = nullptr;
};

TfheBitExec::TfheBitExec(std::string p_filename, KeyType p_keytype)
    : tfhe_context(new TfheContext(p_filename, p_keytype)) {
}

ObjHandle TfheBitExec::encode(const bit_plain_t pt_val) {
  ObjHandleT<LweSample> hdl = new_handle();
  bootsCONSTANT(hdl.get(), pt_val, tfhe_context->pk());
  return hdl;
}

ObjHandle TfheBitExec::encrypt(const bit_plain_t pt_val) {
  ObjHandleT<LweSample> hdl = new_handle();
  bootsSymEncrypt(hdl.get(), pt_val%2, tfhe_context->sk());
  return hdl;
}

IBitExec::bit_plain_t TfheBitExec::decrypt(const ObjHandle& in) {
  return (bit_plain_t)bootsSymDecrypt(in.get<LweSample>(), tfhe_context->sk());
}

ObjHandle TfheBitExec::read(const std::string& name) {
  ifstream file(name);
  if (not file.is_open()) {
    cerr << "ERROR TfheBitExec::read: Cannot open file '" << name << "'" << endl;
    abort();
  }

  ObjHandleT<LweSample> hdl = new_handle();
  import_gate_bootstrapping_ciphertext_fromStream(file, hdl.get(),
                                                  tfhe_context->params());
  file.close();

  return hdl;
}

void TfheBitExec::write(const ObjHandle& in, const std::string& name) {
  ofstream file(name);
  if (not file.is_open()) {
    cerr << "ERROR TfheBitExec::write : cannot open file '" << name << "'" << endl;
    abort();
  }
  export_gate_bootstrapping_ciphertext_toStream(file, in.get<LweSample>(),
                                                tfhe_context->params());
}

ObjHandle TfheBitExec::op_not(const ObjHandle& in) {
  ObjHandleT<LweSample> hdl = new_handle();
  bootsNOT(hdl.get(), in.get<LweSample>(), tfhe_context->pk());
  return hdl;
}

#define TFHE_EXEC_OPER(OPER, TFHE_FNC)                                         \
  ObjHandle TfheBitExec::OPER(const ObjHandle &in1, const ObjHandle &in2) {   \
    ObjHandleT<LweSample> hdl = new_handle();                                  \
    TFHE_FNC(hdl.get(), in1.get<LweSample>(), in2.get<LweSample>(),            \
             tfhe_context->pk());                                                   \
    return hdl;                                                                \
  }

TFHE_EXEC_OPER(op_and,    bootsAND);
TFHE_EXEC_OPER(op_xor,    bootsXOR);
TFHE_EXEC_OPER(op_nand,   bootsNAND);
TFHE_EXEC_OPER(op_andyn,  bootsANDYN);
TFHE_EXEC_OPER(op_andny,  bootsANDNY);
TFHE_EXEC_OPER(op_or,     bootsOR);
TFHE_EXEC_OPER(op_nor,    bootsNOR);
TFHE_EXEC_OPER(op_oryn,   bootsORYN);
TFHE_EXEC_OPER(op_orny,   bootsORNY);
TFHE_EXEC_OPER(op_xnor,   bootsXNOR);

ObjHandle TfheBitExec::op_mux(const ObjHandle &cond, const ObjHandle &in1,
                               const ObjHandle &in2) {
  ObjHandleT<LweSample> hdl = new_handle();
  bootsMUX(hdl.get(), cond.get<LweSample>(), in1.get<LweSample>(),
           in2.get<LweSample>(), tfhe_context->pk());
  return hdl;
}

void* TfheBitExec::new_obj() {
  return new_gate_bootstrapping_ciphertext(tfhe_context->params());
}

void TfheBitExec::del_obj(void * obj_ptr) {
  delete_gate_bootstrapping_ciphertext((LweSample*)obj_ptr);
}


