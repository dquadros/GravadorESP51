/**
 * Programador de ATS89S51 usando um ESP32
 * 
 * Fev/21 Daniel Quadros
 */

#include <WiFi.h>          
#include <WebServer.h>
#include <AutoConnect.h>

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <SPI.h>

#include "Gravador.h"

// Display de 2 linhas de 16 colunas
// ligado via I2C
LiquidCrystal_I2C lcd (0x27, 16, 2);

// Tratamento do WiFi
WebServer Server;          
AutoConnect Portal(Server);

// Firmware a gravar
const int tamFlash = 4*1024;
byte firmware[tamFlash];
int tamFw = 0;

// Página para carga do firmware a gravar
static const char PAGE_UPLOAD[] PROGMEM = R"(
{
  "uri": "/",
  "title": "Carga do Firmware",
  "menu": true,
  "element": [
    { "name":"caption", "type":"ACText", "value":"<h2>Carga do firmware a gravar<h2>" },
    { "name":"upload_file", "type":"ACFile", "label":"Escolha o arquivo hex: ", "store":"extern" },
    { "name":"upload", "type":"ACSubmit", "value":"CARREGA", "uri":"/result" }
  ]
}
)";

// Página de resultado da carga e disparo da gravação
static const char PAGE_RESULT[] PROGMEM = R"(
{
  "uri": "/result",
  "title": "Gravacao do Firmware",
  "menu": false,
  "element": [
    { "name":"caption", "type":"ACText", "format":"<h2>%s<h2>" },
    { "name":"filename", "type":"ACText" },
    { "name":"size", "type":"ACText", "format":"Tamanho do firmware: %s" },
    { "name":"gravar", "type":"ACSubmit", "value":"GRAVAR", "uri":"/flash" }
  ]
}
)";

// Página de resultado da gravação
static const char PAGE_FLASH[] PROGMEM = R"(
{
  "uri": "/flash",
  "title": "Firmware Gravado",
  "menu": false,
  "element": [
    { "name":"caption", "type":"ACText", "format":"<h2>%s<h2>" },
    { "name":"voltar", "type":"ACSubmit", "value":"Voltar", "uri":"/result" }
  ]
}
)";

// Variáveis para tratamento das páginas
AutoConnectAux auxUpload;
AutoConnectAux auxResult;
AutoConnectAux auxFlash;
CustomUploader uploader;

// Iniciação
void setup() {
  grvReset();
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Pronto");

  lcd.init();
  lcd.backlight();

  auxUpload.load(PAGE_UPLOAD);
  auxResult.load(PAGE_RESULT);
  auxFlash.load(PAGE_FLASH);
  Portal.join({ auxUpload, auxResult, auxFlash });  
  auxResult.onUpload<CustomUploader>(uploader);
  auxResult.on(postUpload);
  auxFlash.on(postFlash);
  
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    infoLCD ("Pronto");
  }
}

// Laço principal
void loop() {
   Portal.handleClient();
}

// Rotina executada após o POST da página de upload
String postUpload(AutoConnectAux& aux, PageArgument& args) {

  Serial.println("postUpload");

  AutoConnectText&  aux_caption = aux["caption"].as<AutoConnectText>();
  AutoConnectFile&  upload = auxUpload["upload_file"].as<AutoConnectFile>();
  AutoConnectText&  aux_filename = aux["filename"].as<AutoConnectText>();
  AutoConnectText&  aux_size = aux["size"].as<AutoConnectText>();

  aux_filename.value = upload.value;
  aux_size.value = String(tamFw);
  aux_caption.value = "Arquivo Carregado";
  
  return String();
}

// Rotina executada após o POST da página de gravação
String postFlash(AutoConnectAux& aux, PageArgument& args) {
  Serial.println("postGravar");
  AutoConnectText&  aux_caption = aux["caption"].as<AutoConnectText>();
  if (tamFw == 0) {
      aux_caption.value = "Carregue um firware primeiro";
  } else {
    switch (grvGrava()) {
      case Sucesso:
        aux_caption.value = "Gravacao bem sucedida";
        break;
      case ErroProg:
        aux_caption.value = "Nao conseguiu iniciar gravacao";
        break;
      case ErroId:
        aux_caption.value = "Nao eh um AT89S51";
        break;
      case ErroErase:
        aux_caption.value = "Erro ao apagar";
        break;
      case ErroGravacao:
        aux_caption.value = "Erro ao gravar";
        break;
      case ErroVerificacao:
        aux_caption.value = "Erro ao verificar";
        break;
    }
  }  
  return String();
}

// Mostra uma mensagem na segunda linha do disply
// Na primeira linha vai sempre o endereço IP
void infoLCD(char *msg) {
    lcd.clear();
    lcd.print(WiFi.localIP().toString());
    lcd.setCursor(0, 1);
    lcd.print(msg);
}
