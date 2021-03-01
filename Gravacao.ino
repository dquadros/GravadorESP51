/**
 * Gravação do AT89S51
 */

// O clock do SPI não pode ultrapassar 1/16 do cristal do ATS8951 (12MHz)
const uint32_t clockSPI = 300000L;

// Iniciação após reset
void grvReset() {
  // Desliga a alimentação no soquete do AT89S51
  pinMode(pinPower51, OUTPUT);
  digitalWrite(pinPower51, LOW);
}

// Efetua o processo de gravação do firmware
ResultGravacao grvGrava () {
  ResultGravacao ret = Sucesso;
  target_poweron();
  if (start_pmode()) {
    uint32_t id = read_chipid();
    if (id == 0x1E5106) {
      boolean limpa = is_flashempty();
      if (!limpa) {
        Serial.println ("Apagando a Flash");
        infoLCD ("Apagando");
        if (clear_flash() && is_flashempty()) {
          Serial.println ("Flash apagada");
          limpa = true;
        } else {
          ret = ErroErase;
        }
      }
      if (limpa) {
        infoLCD ("Gravando");
        if (write_flash()) {
          infoLCD ("Verificando");
          if (verify_flash()) {
            Serial.println ("Programada com sucesso");
          } else {
            ret = ErroVerificacao;
          }
        } else {
            ret = ErroGravacao;
        }
      }
    } else {
      Serial.print ("Id incorreto: ");
      Serial.println (id, HEX);
      ret = ErroId;
    }
  } else {
    ret = ErroProg;
  }
  target_poweroff();

  if (ret == Sucesso) {
     infoLCD ("Sucesso");
  } else {
     infoLCD ("Erro");
  }
  return ret;
}

// Grava firmware na flash
// (chamar target_poweron e start_pmode antes)
boolean write_flash() {
  Serial.println ("Programando a Flash");

  for (uint16_t addr = 0; addr < tamFw; addr++) {
    uint8_t dado = firmware[addr];
    
    // dispara a gravação
    SPI.transfer(0x40);
    SPI.transfer((addr >> 8) & 0x0F); 
    SPI.transfer(addr & 0xFF); 
    SPI.transfer(dado);
    
    // "Data polling" espera final da gravação
    uint32_t to = millis() + 2;
    uint8_t lido = ~dado;
    while ((dado != lido) && (to > millis())) {
      lido = read_flash(addr);
    }
    if (dado != lido) {
        Serial.print ("Erro no endereco 0x");
        Serial.print (addr, HEX);
        Serial.print (" gravado 0x");
        Serial.print (dado, HEX);
        Serial.print (" lido 0x");
        Serial.println (lido, HEX);
      return false;
    }
  }
  
  return true;
}

// Verifica a gravação da flash
// (chamar target_poweron e start_pmode antes)
boolean verify_flash() {
  boolean ret = true;
  for (int page = 0; ret && (page < 16); page++) {
    // Seleciona a pagina
    SPI.transfer(0x30); 
    SPI.transfer(page); 
    // Lê os bytes da página
    for (int b = 0; b < 256; b++) {
      uint16_t addr = (page << 8) + b;
      uint8_t dado = SPI.transfer(0);
      if (dado != firmware[addr]) {
        Serial.print ("Erro no endereco 0x");
        Serial.print (addr, HEX);
        Serial.print (" gravado 0x");
        Serial.print (firmware[addr], HEX);
        Serial.print (" lido 0x");
        Serial.println (dado, HEX);
        ret = false;
      }
    }
  }
  return ret;
}

// Verifica se a Flash está limpa
// (chamar target_poweron e start_pmode antes)
boolean is_flashempty() {
  boolean ret = true;
  for (int page = 0; ret && (page < 16); page++) {
    // Seleciona a pagina
    SPI.transfer(0x30); 
    SPI.transfer(page); 
    // Lê os bytes da página
    for (int b = 0; b < 256; b++) {
      if (SPI.transfer(0) != 0xFF) {
        //Serial.print ("Erro no endereco 0x");
        //Serial.println ((page << 8) + b);
        ret = false;
      }
    }
  }
  if (ret) {
    Serial.println ("Flash limpa");
  } else {
    Serial.println ("Flash suja");
  }
  return ret;
}

// Apaga a Flash
// (chamar target_poweron e start_pmode antes)
boolean clear_flash() {
  // dispara o apagamento
  SPI.transfer(0xAC); 
  SPI.transfer(0x80); 
  SPI.transfer(0x00); 
  SPI.transfer(0x00);

  // espera apagar
  uint32_t to = millis() + 2000;
  uint8_t dado = 0;
  while ((dado != 0xFF) && (to > millis())) {
    dado = read_flash(0xFFF);
  }
  if (dado != 0xFF) {
    Serial.println ("Timeout no apagamento");
  }
  return dado == 0xFF;
}

// Lê uma posição da flash
// (chamar target_poweron e start_pmode antes)
uint8_t read_flash(uint16_t addr) {
  SPI.transfer(0x20);
  SPI.transfer((addr >> 8) & 0x0F); 
  SPI.transfer(addr & 0xFF); 
  return SPI.transfer(0x00); 
}

// Le a identificação do chip
// (chamar target_poweron e start_pmode antes)
uint32_t read_chipid() {
  return (read_sigbyte(0x000) << 16) | (read_sigbyte(0x100) << 8) | read_sigbyte(0x200);
}

// Lê um byte de assinatura do chip
uint8_t read_sigbyte(uint16_t addr) {
  SPI.transfer(0x28); 
  SPI.transfer((addr >> 8) & 0x0F); 
  SPI.transfer(addr & 0x80); 
  return SPI.transfer(0x00); 
}

// Coloca o AT89S51 no modo de programação
// (chamar target_poweron primeiro)
boolean start_pmode () {
  SPI.transfer(0xAC); 
  SPI.transfer(0x53); 
  SPI.transfer(0x00); 
  byte ret = SPI.transfer(0x00); 
  if (ret != 0x69) {
    Serial.println ("Erro ao entrar no modo programacao");
    return false;
  }
  Serial.println ("Entrou no modo programacao");
  return true;
}

// Alimenta o AT89S51
void target_poweron ()
{
  SPI.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_CS0);
  SPI.setFrequency(clockSPI);
  delay(200);
  digitalWrite(pinPower51, HIGH);
  delay(200);
  Serial.println ("ATS8951 ligado");
}

// Desliga o AT89S51
void target_poweroff ()
{
  Serial.println ("desligando ATS8951");
  digitalWrite(pinPower51, LOW);
  SPI.end();
  delay (100);
  Serial.println ("ATS8951 desligado");
}
