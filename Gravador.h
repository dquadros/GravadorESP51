// Conexões ao AT89S51
const int VSPI_MOSI = 23;
const int VSPI_MISO = 19;
const int VSPI_SCK = 18;
const int VSPI_CS0 = 5;
const int pinPower51 = VSPI_CS0;

// Resultados da Gravação
typedef enum { 
  Sucesso,          // Sucesso
  ErroProg,         // Não conseguiu colocar no modo programação
  ErroId,           // Id do chip não é do AT80S51
  ErroErase,        // Erro ao apagar
  ErroGravacao,     // Erro ao gravar
  ErroVerificacao   // Erro ao verificar
} ResultGravacao;

// Custom upload handler
class CustomUploader : public AutoConnectUploadHandler {
public:
  CustomUploader() {}
  ~CustomUploader() {}

private:
  byte linha[100];
  int pos;

protected:
  bool   _open(const char* filename, const char* mode) override;
  size_t _write(const uint8_t *buf, const size_t size) override;
  void   _close(const HTTPUploadStatus status) override;
};
