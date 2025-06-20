/*
 * Pong
 * Original Code from https://github.com/rparrett/pongclock
 * Modified by Ing. Di Filippo Giugno 2025
 */

#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREY  0x5AEB

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

int16_t h = 128;
int16_t w = 160;

int dly = 20;

int16_t paddle_h = 25;
int16_t paddle_w = 2;

int16_t lpaddle_x = 0;
int16_t rpaddle_x = w - paddle_w;

int16_t lpaddle_y = 0;
int16_t rpaddle_y = h - paddle_h;

int16_t lpaddle_d = 1;
int16_t rpaddle_d = -1;

int16_t lpaddle_ball_t = w - w / 4;
int16_t rpaddle_ball_t = w / 4;

int16_t target_y = 0;

int16_t ball_x = 2;
int16_t ball_y = 2;
int16_t oldball_x = 2;
int16_t oldball_y = 2;

int16_t ball_dx = 1;
int16_t ball_dy = 1;

int16_t ball_w = 4;
int16_t ball_h = 4;

int16_t dashline_h = 4;
int16_t dashline_w = 2;
int16_t dashline_n = h / dashline_h;
int16_t dashline_x = w / 2 - 1;
int16_t dashline_y = dashline_h / 2;



// Punteggi iniziali
int16_t lscore = 0; // Punteggio del giocatore (tu)
int16_t rscore = 0; // Punteggio dell'avversario

// --- Definizioni per i pulsanti ---
#define BUTTON_UP_PIN 2 // GP2 per spostare il paddle in su
#define BUTTON_DOWN_PIN 3 // GP3 per spostare il paddle in giù

// Variabili per il debounce dei pulsanti (opzionale ma consigliato)
unsigned long lastButtonUpTime = 0;
unsigned long lastButtonDownTime = 0;
const long debounceDelay = 50; // Tempo di debounce in millisecondi
// --- Fine delle nuove definizioni per i pulsanti ---

void setup(void) {
  
  randomSeed(analogRead(0)*analogRead(1));
    
  tft.init();

  tft.setRotation(1);

  tft.fillScreen(BLACK);
  
  // --- Inizio configurazione GPIO per i pulsanti ---
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);    // Imposta GP2 come input con pull-up interno
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);  // Imposta GP3 come input con pull-up interno
  // --- Fine configurazione GPIO per i pulsanti ---

  initgame();

  tft.setTextColor(WHITE, BLACK);

  delay(2000);
  
}

void loop() {
  delay(dly);

  // --- Inizio della logica di controllo del paddle sinistro con i pulsanti ---
  if (digitalRead(BUTTON_UP_PIN) == LOW) { // Se il pulsante UP è premuto (LOW a causa del pull-up)
    if (millis() - lastButtonUpTime > debounceDelay) {
      lpaddle_y -= 3; // Sposta il paddle verso l'alto. Puoi aggiustare il valore per cambiare la velocità.
      if (lpaddle_y < 0) lpaddle_y = 0; // Impedisce al paddle di andare oltre il bordo superiore
      lastButtonUpTime = millis();
    }
  }

  if (digitalRead(BUTTON_DOWN_PIN) == LOW) { // Se il pulsante DOWN è premuto
    if (millis() - lastButtonDownTime > debounceDelay) {
      lpaddle_y += 3; // Sposta il paddle verso il basso. Puoi aggiustare il valore per cambiare la velocità.
      if (lpaddle_y + paddle_h > h) lpaddle_y = h - paddle_h; // Impedisce al paddle di andare oltre il bordo inferiore
      lastButtonDownTime = millis();
    }
  }
  // --- Fine della logica di controllo del paddle sinistro con i pulsanti ---

  lpaddle(); // La funzione lpaddle verrà modificata per non muovere autonomamente
  rpaddle();

  midline();

  ball();
  displayScores();
}

void initgame() {
  tft.fillScreen(BLACK);

  lpaddle_y = random(0, h - paddle_h);
  rpaddle_y = random(0, h - paddle_h);

  // ball is placed on the center of the left paddle
  ball_y = lpaddle_y + (paddle_h / 2);
  
  calc_target_y();

  midline();

  tft.fillRect(0,h-26,w,h-1,BLACK);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(WHITE);
  tft.drawString("ITIS Pong 2025", w/2, h-26 , 2);


}

void midline() {

  // If the ball is not on the line then don't redraw the line
  if ((ball_x<dashline_x-ball_w) && (ball_x > dashline_x+dashline_w)) return;

  tft.startWrite();

  // Quick way to draw a dashed line
  tft.setAddrWindow(dashline_x, 0, dashline_w, h);
  
  for(int16_t i = 0; i < dashline_n; i+=2) {
    tft.pushColor(TFT_YELLOW, dashline_w*dashline_h); // push dash pixels
    tft.pushColor(TFT_BLACK, dashline_w*dashline_h); // push gap pixels
  }

  tft.endWrite();
}

void lpaddle() {
  // Questa funzione ora si occupa solo di disegnare il paddle nella posizione corrente
  // La logica di movimento è stata spostata nella loop() e gestita dai pulsanti

  // Cancella il paddle nella sua vecchia posizione
  tft.fillRect(lpaddle_x, 0, paddle_w, h, BLACK); // Cancella tutta la colonna del paddle

  // Disegna il paddle nella nuova posizione
  tft.fillRect(lpaddle_x, lpaddle_y, paddle_w, paddle_h, WHITE);
}

void rpaddle() {
  // Il paddle destro continua a muoversi autonomamente come prima
  if (rpaddle_d == 1) {
    tft.fillRect(rpaddle_x, rpaddle_y, paddle_w, 1, BLACK);
  } 
  else if (rpaddle_d == -1) {
    tft.fillRect(rpaddle_x, rpaddle_y + paddle_h - 1, paddle_w, 1, BLACK);
  }

  rpaddle_y = rpaddle_y + rpaddle_d;

  if (ball_dx == -1) rpaddle_d = 0;
  else {
    if (rpaddle_y + paddle_h / 2 == target_y) rpaddle_d = 0;
    else if (rpaddle_y + paddle_h / 2 > target_y) rpaddle_d = -1;
    else rpaddle_d = 1;
  }

  if (rpaddle_y + paddle_h >= h && rpaddle_d == 1) rpaddle_d = 0;
  else if (rpaddle_y <= 0 && rpaddle_d == -1) rpaddle_d = 0;

  tft.fillRect(rpaddle_x, rpaddle_y, paddle_w, paddle_h, WHITE);
}

void calc_target_y() {
  int16_t target_x;
  int16_t reflections;
  int16_t y;

  if (ball_dx == 1) {
    target_x = w - ball_w;
  } 
  else {
    target_x = -1 * (w - ball_w);
  }

  y = abs(target_x * (ball_dy / ball_dx) + ball_y);

  reflections = floor(y / h);

  if (reflections % 2 == 0) {
    target_y = y % h;
  } 
  else {
    target_y = h - (y % h);
  }
}

void ball() {
  ball_x = ball_x + ball_dx;
  ball_y = ball_y + ball_dy;

  if (ball_dx == -1 && ball_x == paddle_w && ball_y + ball_h >= lpaddle_y && ball_y <= lpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;
    //dly = random(5); // change speed of ball after paddle contact
    calc_target_y(); 
  } else if (ball_dx == 1 && ball_x + ball_w == w - paddle_w && ball_y + ball_h >= rpaddle_y && ball_y <= rpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;
    //dly = random(5); // change speed of ball after paddle contact
    calc_target_y();
  } else if ((ball_dx == 1 && ball_x >= w) || (ball_dx == -1 && ball_x + ball_w < 0)) {
    if (ball_x >= w) { // La pallina è uscita a destra (punto per il giocatore)
    lscore++;
    ball_dx = -1;
  } else { // La pallina è uscita a sinistra (punto per l'avversario)
    ball_dx = 1;
    rscore++;
  }

  delay(1000); // Breve pausa prima di reinizializzare la pallina
  initgame(); // Reinizializza la posizione di pallina e paddle per un nuovo round
  return; // Esci dalla funzione ball() per non disegnare la pallina in una posizione sbagliata
  }

  if (ball_y > h - ball_w || ball_y < 0) {
    ball_dy = ball_dy * -1;
    ball_y += ball_dy; // Keep in bounds
  }

  tft.drawRect(oldball_x, oldball_y, ball_w, ball_h, BLACK); // Less TFT refresh aliasing than line above for large balls
  tft.fillRect(    ball_x,    ball_y, ball_w, ball_h, TFT_ORANGE);
  oldball_x = ball_x;
  oldball_y = ball_y;
}

void displayScores() {
  tft.setTextDatum(TL_DATUM); // Imposta l'allineamento del testo in alto a sinistra
  tft.setTextColor(TFT_YELLOW, BLACK); // Colore testo bianco, sfondo nero

  // Pulisci l'area dei punteggi se il punteggio deve essere aggiornato
  //tft.fillRect(paddle_w, 0, (w/2)- 2*dashline_w, tft.fontHeight(2) + 2, BLACK); // Pulisci la riga superiore

  // Conversione del punteggio in stringa per textWidth()
  char score_str_l[2]; // Per un punteggio da 0 a 9
  char score_str_r[2];
  sprintf(score_str_l, "%d", lscore);
  sprintf(score_str_r, "%d", rscore);

  // Punteggio giocatore (a sinistra)
  tft.drawString(score_str_l, w/4 - tft.textWidth(score_str_l, 2)/2, 2, 2); // Mostra il punteggio del giocatore

  // Punteggio avversario (a destra)
  tft.drawString(score_str_r, w * 3/4 - tft.textWidth(score_str_r, 2)/2, 2, 2); // Mostra il punteggio dell'avversario
}
