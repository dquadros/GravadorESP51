/*
 * Classe personalizada para tratar o upload de arquivo
 */

// Inicia o tratamento
bool CustomUploader::_open(const char* filename, const char* mode) {
  Serial.println("open");
  memset (firmware, 0xFF, tamFlash);
  tamFw = 0;
  pos = 0;
  return true;
}

// Rotina auxiliar para converte tam dígitos hexadecimais em um valor inteiro
uint16_t decodHex (byte *p, int tam) {
  uint16_t ret = 0;
  for (int i = 0; i < tam; i++) {
    byte c = *p++;
    ret = ret << 4;
    if ((c >= '0') && (c <= '9')) {
      ret += c - '0';
    } else if ((c >= 'A') && (c <= 'F')) {
      ret += c - 'A' + 10;
    } else if ((c >= 'a') && (c <= 'f')) {
      ret += c - 'a' + 10;
    }
  }
  return ret;
}

// Trata um bloco de dados do arquivo
size_t CustomUploader::_write(const uint8_t *buf, const size_t size) {
  Serial.println("write");
  for (int i = 0; i < size; i++) {
    byte c = buf[i];
    if (c == '\n') {
      // fim de uma linha, processar se
      // linha começa com ':', tem mais de 10 caracteres e tem número ímpar de caracteres
      if ((pos > 10) && (linha[0] == ':') && ((pos & 1) == 1)) {
        // Extrai tamanho, endereço e tipo
        // Vamos ignorar o checksum e assumir que o tamanho está correto
        byte tam = (byte) decodHex (linha+1, 2);
        uint16_t addr = decodHex (linha+3, 4);
        byte tipo = (byte) decodHex (linha+7, 2);
        Serial.print(tam, HEX); Serial.print(" ");
        Serial.print(addr, HEX); Serial.print(" ");
        Serial.println(tipo, HEX);
        if (tipo == 0) {
          for (int j = 0; j < tam; j++) {
            byte dado = (byte) decodHex (linha+9+2*j, 2);
            if (addr < tamFlash) {
              firmware[addr] =  dado;
              if (addr > tamFw) {
                tamFw = addr;       // lembra a última posição escrita
              }
            }
            addr++;
          }
        }
      }
      pos = 0;  // iniciar nova linha
    } else if ((c > 0x20 ) && (c < 0x7F) && (pos < sizeof(linha))) {
      linha[pos++] = c;
    }
  }
  return size;
}

// Encerra o tratamento do arquivo
void CustomUploader::_close(const HTTPUploadStatus status) {
  Serial.println("close");
  tamFw++;  // tamFw estava com o ultimo endereco usado
  // Dump para debug
  for (int addr = 0; addr < tamFw; addr += 16) {
    Serial.print (addr, HEX);
    for (int i = 0; i < 16; i++) {
      Serial.print (" ");
      Serial.print (firmware[addr+i], HEX);      
    }
    Serial.println();
  }
}
