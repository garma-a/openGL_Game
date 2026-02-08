#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// Sound effects
Mix_Chunk *shootSound = nullptr;
Mix_Chunk *zombieDeathSound = nullptr;
Mix_Chunk *pickupSound = nullptr;
Mix_Chunk *freezeSound = nullptr;
Mix_Chunk *gameOverSound = nullptr;
Mix_Chunk *levelUpSound = nullptr;
Mix_Chunk *hitSound = nullptr;
Mix_Music *backgroundMusic = nullptr;
bool soundEnabled = true;

void levelCountdownTimer(int value);
void loadZombies(int count);
void update(int value);
void playSound(Mix_Chunk *sound);
void drawCircle(float cx, float cy, float r);
void reshape(int width, int height);

struct Bullet {
  float x, y;
  bool active;
  int damage;
  float r, g, b;
};

struct Zombie {
  float x, y;
  bool isAlive;
  int health;
  float angle;
  float baseY;
  bool zigzagEnabled;
};

struct HealthPickup {
  float x, y;
  bool active;
  int healAmount;
  float radius;
};

struct FreezePickup {
  float x, y;
  bool active;
  float radius;
};

struct Assistant {
  float y;
  bool active;
  float lastShotTime;
};

enum UpgradeType {
  UPGRADE_NONE,
  UPGRADE_SERVER,
  UPGRADE_BULLET_ORANGE,
  UPGRADE_BULLET_RED
};

const float serverLeft = 0.7f;
const float serverRight = 1.0f;
const float hackerLeft = -1.0f;
const float hackerRight = -0.7f;

float lastZombieEntryTime = 0.0f;
float dangerWindowRemaining = 0.0f;

int points = 0;
bool serverUpgraded = false;
std::vector<float> zombieEntryTimes;
float upgradeEndTime = 0.0f;
const float upgradeCost = 20;
const float upgradeDuration = 10.0f;

std::vector<Bullet> bullets;
std::vector<Zombie> zombies;
std::vector<HealthPickup> healthPickups;
std::vector<FreezePickup> freezePickups;

bool zombiesFrozen = false;
float freezeEndTime = 0.0f;
const float freezeDuration = 5.0f;
const float freezePickupRadius = 0.03f;
const float freezePickupDropChance = 0.2f;

// Assistant variables
std::vector<Assistant> assistants;
const float assistantShotInterval = 1.0f;
const int assistantCost = 40;

float zombieBaseSpeed = 0.002f;
float playerY = 0.0f;
bool keyUpPressed = false;
bool keyDownPressed = false;

const int screenWidth = 1200;
const int screenHeight = 900;

bool gameOver = false;
int level = 1;
int zombiesKilled = 0;
int maxLevel = 20;

enum Difficulty { DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD };

Difficulty currentDifficulty = DIFFICULTY_MEDIUM;
float difficultySpeedMultiplier = 1.0f;
int zombieDamage = 28;

const int orangeUpgradeCost = 10;
const int redUpgradeCost = 30;
int bulletUpgradeLevel = 0;

bool shopOpen = false;
bool difficultySelectionActive = true;

int maxAmmo = 20;
int currentAmmo = 20;
bool reloading = false;
float lastReloadTime = 0.0f;

int playerMaxHealth = 100;
int playerHealth = 100;
const int healthPickupHeal = 350;
const float healthPickupRadius = 0.03f;
const float healthPickupDropChance = 0.35f;

bool levelCountdownActive = false;
int levelCountdownValue = 5;

void drawGameOverMenu();
void drawDifficultyScreen();

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// Remove custom defines that conflict with system headers
// Use simple console logging for text in web version to avoid link errors
#endif

// ...

// Custom vector font renderer for WebAssembly compatibility
void drawCustomChar(char c, float x, float y, float size) {
  if (c >= 'a' && c <= 'z') {
    c = static_cast<char>(c - 'a' + 'A');
  }
  glPushMatrix();
  glTranslatef(x, y, 0.0f);
  glScalef(size, size, 1.0f);
  glBegin(GL_LINES);
#define LINE(x1, y1, x2, y2)                                                   \
  glVertex2f((x1), (y1));                                                      \
  glVertex2f((x2), (y2));

  // Clean, thin stroke font in a 1x1 box
  if (c == 'A') {
    LINE(0.0f, 0.0f, 0.5f, 1.0f);
    LINE(1.0f, 0.0f, 0.5f, 1.0f);
    LINE(0.2f, 0.5f, 0.8f, 0.5f);
  } else if (c == 'B') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.0f, 0.5f);
    LINE(0.8f, 0.5f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
  } else if (c == 'C') {
    LINE(0.8f, 1.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == 'D') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
  } else if (c == 'E') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.0f, 0.5f, 0.6f, 0.5f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == 'F') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.0f, 0.5f, 0.6f, 0.5f);
  } else if (c == 'G') {
    LINE(0.8f, 1.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.4f, 0.5f);
  } else if (c == 'H') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.8f, 0.0f, 0.8f, 1.0f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
  } else if (c == 'I') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.4f, 1.0f, 0.4f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == 'J') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.2f);
    LINE(0.8f, 0.2f, 0.2f, 0.0f);
    LINE(0.2f, 0.0f, 0.0f, 0.2f);
  } else if (c == 'K') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 0.5f, 0.8f, 1.0f);
    LINE(0.0f, 0.5f, 0.8f, 0.0f);
  } else if (c == 'L') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == 'M') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(1.0f, 0.0f, 1.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.5f, 0.4f);
    LINE(1.0f, 1.0f, 0.5f, 0.4f);
  } else if (c == 'N') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.8f, 0.0f, 0.8f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 0.0f);
  } else if (c == 'O' || c == '0') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
  } else if (c == 'P') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.0f, 0.5f);
  } else if (c == 'Q') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
    LINE(0.5f, 0.5f, 0.9f, 0.0f);
  } else if (c == 'R') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.0f, 0.5f);
    LINE(0.0f, 0.5f, 0.8f, 0.0f);
  } else if (c == 'S') {
    LINE(0.8f, 1.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.0f, 0.5f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
  } else if (c == 'T') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.4f, 1.0f, 0.4f, 0.0f);
  } else if (c == 'U') {
    LINE(0.0f, 1.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.8f, 1.0f);
  } else if (c == 'V') {
    LINE(0.0f, 1.0f, 0.4f, 0.0f);
    LINE(0.8f, 1.0f, 0.4f, 0.0f);
  } else if (c == 'W') {
    LINE(0.0f, 1.0f, 0.25f, 0.0f);
    LINE(0.25f, 0.0f, 0.5f, 0.6f);
    LINE(0.5f, 0.6f, 0.75f, 0.0f);
    LINE(0.75f, 0.0f, 1.0f, 1.0f);
  } else if (c == 'X') {
    LINE(0.0f, 1.0f, 0.8f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 1.0f);
  } else if (c == 'Y') {
    LINE(0.0f, 1.0f, 0.4f, 0.6f);
    LINE(0.8f, 1.0f, 0.4f, 0.6f);
    LINE(0.4f, 0.6f, 0.4f, 0.0f);
  } else if (c == 'Z') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  }

  else if (c == '1') {
    LINE(0.4f, 0.0f, 0.4f, 1.0f);
    LINE(0.2f, 0.0f, 0.6f, 0.0f);
  } else if (c == '2') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == '3') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  } else if (c == '4') {
    LINE(0.0f, 1.0f, 0.0f, 0.5f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
  } else if (c == '5') {
    LINE(0.8f, 1.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.0f, 0.5f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
  } else if (c == '6') {
    LINE(0.8f, 1.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.8f, 0.5f);
    LINE(0.8f, 0.5f, 0.0f, 0.5f);
  } else if (c == '7') {
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.2f, 0.0f);
  } else if (c == '8') {
    LINE(0.0f, 0.0f, 0.0f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.8f, 1.0f, 0.8f, 0.0f);
    LINE(0.8f, 0.0f, 0.0f, 0.0f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
  } else if (c == '9') {
    LINE(0.8f, 0.0f, 0.8f, 1.0f);
    LINE(0.0f, 1.0f, 0.8f, 1.0f);
    LINE(0.0f, 0.5f, 0.8f, 0.5f);
    LINE(0.0f, 1.0f, 0.0f, 0.5f);
    LINE(0.0f, 0.0f, 0.8f, 0.0f);
  }

  else if (c == ':') {
    LINE(0.45f, 0.7f, 0.55f, 0.7f);
    LINE(0.45f, 0.3f, 0.55f, 0.3f);
  } else if (c == '-') {
    LINE(0.1f, 0.5f, 0.7f, 0.5f);
  } else if (c == '!') {
    LINE(0.4f, 0.2f, 0.4f, 1.0f);
    LINE(0.4f, 0.05f, 0.4f, 0.1f);
  } else if (c == '.') {
    LINE(0.45f, 0.05f, 0.55f, 0.05f);
  } else if (c == ',') {
    LINE(0.5f, 0.05f, 0.4f, -0.1f);
  } else if (c == '/') {
    LINE(0.0f, 0.0f, 0.8f, 1.0f);
  } else if (c == '(') {
    LINE(0.6f, 1.0f, 0.2f, 0.5f);
    LINE(0.2f, 0.5f, 0.6f, 0.0f);
  } else if (c == ')') {
    LINE(0.2f, 1.0f, 0.6f, 0.5f);
    LINE(0.6f, 0.5f, 0.2f, 0.0f);
  }

  // Add missing numbers/chars generically (Box) if not found
  else {
    // Generic box for space/unknown
    if (c != ' ') {
      LINE(0.0f, 0.0f, 0.8f, 0.0f);
      LINE(0.8f, 0.0f, 0.8f, 1.0f);
      LINE(0.8f, 1.0f, 0.0f, 1.0f);
      LINE(0.0f, 1.0f, 0.0f, 0.0f);
    }
  }

#undef LINE

  glEnd();
  glPopMatrix();
}

void drawLevelCountdown() {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", levelCountdownValue);
  glColor3f(1.0f, 1.0f, 0.0f);

#ifdef __EMSCRIPTEN__
  float startX = -0.08f;
  float size = 0.14f;
  float spacing = size * 0.8f;
  for (int i = 0; buf[i] != '\0'; ++i) {
    drawCustomChar(buf[i], startX + (i * spacing), -0.12f, size);
  }
#else
  // Desktop version: Keep original fancy stroke text
  glPushMatrix();
  glTranslatef(-0.15f, -0.15f, 0.0f);
  glScalef(0.0045f, 0.0045f, 1.0f);
  for (char *c = buf; *c; ++c) {
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
  }
  glPopMatrix();
#endif
}

// ...

void renderText(float x, float y, const std::string &text) {
#ifdef __EMSCRIPTEN__
  glColor3f(1, 1, 1);
  float charSize = 0.025f;
  float charSpacing = charSize * 0.85f;
  for (size_t i = 0; i < text.length(); ++i) {
    drawCustomChar(text[i], x + (i * charSpacing), y, charSize);
  }
#else
  glColor3f(1, 1, 1);
  glRasterPos2f(x, y);
  for (char c : text) {
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
  }
#endif
}

void startLevelCountdown() {
  levelCountdownActive = true;
  levelCountdownValue = 5;
  // Remove all active bullets when countdown starts
  bullets.clear();
  glutTimerFunc(1000, levelCountdownTimer, 0);
}

void levelCountdownTimer(int value) {
  if (!levelCountdownActive)
    return;
  levelCountdownValue--;
  if (levelCountdownValue > 0) {
    glutPostRedisplay();
    glutTimerFunc(1000, levelCountdownTimer, 0);
  } else {
    levelCountdownActive = false;
    loadZombies(6 + level); // Start new level zombies
    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // Resume game updates
  }
}

void levelCountdownTimer(int value);

void drawSquare(float x, float y, float width, float height, float r, float g,
                float b, float a = 1) {
  glColor4f(r, g, b, a);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(x, y);
  glVertex2f(x + width, y);
  glVertex2f(x + width, y + height);
  glVertex2f(x, y + height);
  glEnd();
}

void drawAmmoBar() {
  float barWidth = 0.7f;         // Smaller width
  float barHeight = 0.04f;       // Smaller height
  float barX = -barWidth / 2.0f; // Centered horizontally
  float barY = -0.98f;           // Closer to bottom
  float squareGap = 0.008f;
  float squareWidth = (barWidth - (maxAmmo - 1) * squareGap) / maxAmmo;
  float squareHeight = barHeight * 0.8f;
  // Draw bar background
  drawSquare(barX, barY, barWidth, barHeight, 0.2f, 0.2f, 0.2f, 0.7f);
  // Draw ammo squares
  for (int i = 0; i < maxAmmo; ++i) {
    float x = barX + i * (squareWidth + squareGap);
    if (i < currentAmmo)
      drawSquare(x, barY + 0.005f, squareWidth, squareHeight, 1.0f, 1.0f, 0.0f,
                 1.0f); // filled
    else
      drawSquare(x, barY + 0.005f, squareWidth, squareHeight, 0.3f, 0.3f, 0.3f,
                 1.0f); // empty
  }
}

void drawBulletShopIcon(float x, float y, float r, float g, float b) {
  float r2 = r * 0.7f, g2 = g * 0.7f, b2 = b * 0.7f; // Darker shade

  glPushMatrix();
  glTranslatef(x, y, 0.0f);
  glScalef(0.15f, 0.15f, 1.0f);
  glRotatef(90.0f, 0.0f, 0.0f, 1.0f);

  // Exhaust flame
  glColor4f(1.0f, 0.5f, 0.0f, 0.8f);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.01f, -0.02f);
  glVertex2f(0.06f, -0.02f);
  glVertex2f(0.025f, -0.10f);
  glEnd();

  glColor4f(1.0f, 1.0f, 0.3f, 0.9f);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.008f, -0.01f);
  glVertex2f(0.042f, -0.01f);
  glVertex2f(0.025f, -0.06f);
  glEnd();

  // Body
  glColor3f(r2, g2, b2);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(0.05f, 0.0f);
  glVertex2f(0.05f, 0.18f);
  glVertex2f(0.0f, 0.18f);
  glEnd();

  // Body highlight
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.015f, 0.02f);
  glVertex2f(0.035f, 0.02f);
  glVertex2f(0.035f, 0.16f);
  glVertex2f(0.015f, 0.16f);

  glEnd();

  // Nose cone
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.005f, 0.18f);
  glVertex2f(0.055f, 0.18f);
  glVertex2f(0.025f, 0.26f);
  glEnd();

  // Fins
  glColor3f(r2, g2, b2);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(-0.025f, 0.0f);
  glVertex2f(-0.015f, 0.05f);
  glVertex2f(0.0f, 0.07f);
  glEnd();

  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.05f, 0.0f);
  glVertex2f(0.075f, 0.0f);
  glVertex2f(0.065f, 0.05f);
  glVertex2f(0.05f, 0.07f);
  glEnd();

  // Nozzle
  glColor3f(0.25f, 0.25f, 0.3f);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(-0.005f, -0.02f);
  glVertex2f(0.055f, -0.02f);
  glVertex2f(0.05f, 0.02f);
  glVertex2f(0.0f, 0.02f);
  glEnd();

  glPopMatrix();
}

void reloadAmmo(int value) {
  if (currentAmmo < maxAmmo) {
    currentAmmo++;
    glutPostRedisplay();
    glutTimerFunc(600, reloadAmmo, 0); // 0.6 seconds per bullet
  } else {
    reloading = false;
  }
}

void drawShieldShopIcon(float x, float y) {
  glPushMatrix();
  glTranslatef(x, y, 0.0f);
  glScalef(0.06f, 0.06f, 1.0f); // Reduced scale for smaller icon
  glColor3f(0.2f, 0.8f, 1.0f);
  glBegin(GL_TRIANGLE_FAN);
  for (int i = 0; i <= 180; ++i) {
    float theta = M_PI * i / 180.0f;
    glVertex2f(cos(theta), sin(theta));
  }
  glEnd();
  glColor3f(0.1f, 0.4f, 0.7f);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(-1.0f, 0.0f);
  glVertex2f(0.0f, -1.5f);
  glVertex2f(1.0f, 0.0f);
  glEnd();
  glPopMatrix();
}

// Update the bullet rendering logic
void drawBullet(float x, float y) {
  // Set colors based on upgrade level
  float r = 1.0f, g = 1.0f, b = 1.0f;
  float r2 = 0.8f, g2 = 0.8f, b2 = 0.8f; // Secondary color for gradient effect
  float glowR = 1.0f, glowG = 1.0f, glowB = 0.5f; // Glow/flame color

  if (bulletUpgradeLevel == 2) {
    // Red - powerful
    r = 1.0f;
    g = 0.1f;
    b = 0.1f;
    r2 = 0.7f;
    g2 = 0.0f;
    b2 = 0.0f;
    glowR = 1.0f;
    glowG = 0.3f;
    glowB = 0.1f;
  } else if (bulletUpgradeLevel == 1) {
    // Orange - medium
    r = 1.0f;
    g = 0.6f;
    b = 0.1f;
    r2 = 0.9f;
    g2 = 0.4f;
    b2 = 0.0f;
    glowR = 1.0f;
    glowG = 0.8f;
    glowB = 0.2f;
  } else {
    // Default - white/silver
    r = 0.9f;
    g = 0.9f;
    b = 1.0f;
    r2 = 0.7f;
    g2 = 0.7f;
    b2 = 0.8f;
    glowR = 0.8f;
    glowG = 0.9f;
    glowB = 1.0f;
  }

  glPushMatrix();
  glTranslatef(x, y, 0.0f);
  glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
  glScalef(0.5f, 0.5f, 1.0f);

  // === ROCKET EXHAUST FLAME ===
  // Outer flame (orange/yellow)
  glColor4f(1.0f, 0.5f, 0.0f, 0.7f);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.01f, -0.02f);
  glVertex2f(0.06f, -0.02f);
  glVertex2f(0.025f, -0.12f);
  glEnd();

  // Inner flame (bright yellow/white)
  glColor4f(1.0f, 1.0f, 0.3f, 0.9f);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.005f, -0.01f);
  glVertex2f(0.045f, -0.01f);
  glVertex2f(0.025f, -0.07f);
  glEnd();

  // Core flame (white hot)
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.015f, 0.0f);
  glVertex2f(0.035f, 0.0f);
  glVertex2f(0.025f, -0.04f);
  glEnd();

  // === ROCKET BODY ===
  // Main body with metallic look
  glColor3f(r2, g2, b2);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(0.05f, 0.0f);
  glVertex2f(0.05f, 0.18f);
  glVertex2f(0.0f, 0.18f);
  glEnd();

  // Body highlight (metallic shine)
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.015f, 0.02f);
  glVertex2f(0.035f, 0.02f);
  glVertex2f(0.035f, 0.16f);
  glVertex2f(0.015f, 0.16f);
  glEnd();

  // Body stripe detail
  glColor3f(0.3f, 0.3f, 0.35f);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.08f);
  glVertex2f(0.05f, 0.08f);
  glVertex2f(0.05f, 0.10f);
  glVertex2f(0.0f, 0.10f);
  glEnd();

  // === ROCKET NOSE CONE ===
  // Outer nose
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.005f, 0.18f);
  glVertex2f(0.055f, 0.18f);
  glVertex2f(0.025f, 0.28f);
  glEnd();

  // Nose highlight
  glColor3f(1.0f, 1.0f, 1.0f);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.015f, 0.19f);
  glVertex2f(0.035f, 0.19f);
  glVertex2f(0.025f, 0.25f);
  glEnd();

  // === FINS ===
  // Left fin
  glColor3f(r2, g2, b2);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.0f, 0.0f);
  glVertex2f(-0.03f, 0.0f);
  glVertex2f(-0.02f, 0.06f);
  glVertex2f(0.0f, 0.08f);
  glEnd();

  // Left fin highlight
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.005f, 0.02f);
  glVertex2f(-0.02f, 0.02f);
  glVertex2f(-0.01f, 0.05f);
  glEnd();

  // Right fin
  glColor3f(r2, g2, b2);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.05f, 0.0f);
  glVertex2f(0.08f, 0.0f);
  glVertex2f(0.07f, 0.06f);
  glVertex2f(0.05f, 0.08f);
  glEnd();

  // Right fin highlight
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLES);
  glVertex2f(0.055f, 0.02f);
  glVertex2f(0.07f, 0.02f);
  glVertex2f(0.06f, 0.05f);
  glEnd();

  // === ENGINE NOZZLE ===
  glColor3f(0.25f, 0.25f, 0.3f);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(-0.005f, -0.02f);
  glVertex2f(0.055f, -0.02f);
  glVertex2f(0.05f, 0.02f);
  glVertex2f(0.0f, 0.02f);
  glEnd();

  // Nozzle inner
  glColor3f(0.15f, 0.15f, 0.2f);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(0.008f, -0.015f);
  glVertex2f(0.042f, -0.015f);
  glVertex2f(0.04f, 0.01f);
  glVertex2f(0.01f, 0.01f);
  glEnd();

  glPopMatrix();
}

void checkServerBreach() {
  zombieEntryTimes.push_back(glutGet(GLUT_ELAPSED_TIME) / 1000.0f);
}

void purchaseUpgrade() {
  if (points >= upgradeCost) {
    points -= upgradeCost;
    upgradeEndTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f + upgradeDuration;
  }
}

void drawZombie() {
  drawSquare(-0.12f, 0.15f, 0.24f, 0.24f, 0.0f, 0.8f, 0.0f);
  drawSquare(-0.09f, 0.27f, 0.04f, 0.04f, 0.0f, 0.0f, 0.0f);
  drawSquare(0.05f, 0.27f, 0.04f, 0.04f, 0.0f, 0.0f, 0.0f);
  drawSquare(-0.15f, -0.05f, 0.3f, 0.25f, 0.0f, 0.7f, 1.0f);
  drawSquare(-0.27f, -0.05f, 0.1f, 0.25f, 0.0f, 0.8f, 0.0f);
  drawSquare(0.17f, -0.05f, 0.1f, 0.25f, 0.0f, 0.8f, 0.0f);
  drawSquare(-0.10f, -0.30f, 0.07f, 0.20f, 0.1f, 0.0f, 0.6f);
  drawSquare(0.03f, -0.30f, 0.07f, 0.20f, 0.1f, 0.0f, 0.6f);
}

void drawFrozenZombie() {
  drawSquare(-0.12f, 0.15f, 0.24f, 0.24f, 0.4f, 0.7f, 1.0f);
  drawSquare(-0.09f, 0.27f, 0.04f, 0.04f, 0.0f, 0.0f, 0.0f);
  drawSquare(0.05f, 0.27f, 0.04f, 0.04f, 0.0f, 0.0f, 0.0f);
  drawSquare(-0.15f, -0.05f, 0.3f, 0.25f, 0.5f, 0.8f, 1.0f);
  drawSquare(-0.27f, -0.05f, 0.1f, 0.25f, 0.4f, 0.7f, 1.0f);
  drawSquare(0.17f, -0.05f, 0.1f, 0.25f, 0.4f, 0.7f, 1.0f);
  drawSquare(-0.10f, -0.30f, 0.07f, 0.20f, 0.3f, 0.5f, 0.9f);
  drawSquare(0.03f, -0.30f, 0.07f, 0.20f, 0.3f, 0.5f, 0.9f);
}

void drawBackground() {
  // Apocalyptic gradient background - dark purple/red at bottom, dark blue at
  // top
  glBegin(GL_QUADS);
  // Bottom color - dark blood red/purple
  glColor3f(0.15f, 0.02f, 0.05f);
  glVertex2f(-1.0f, -1.0f);
  glVertex2f(1.0f, -1.0f);
  // Top color - dark night blue
  glColor3f(0.02f, 0.02f, 0.08f);
  glVertex2f(1.0f, 1.0f);
  glVertex2f(-1.0f, 1.0f);
  glEnd();

  // Add some atmospheric fog/mist layers
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Ground fog layer
  glBegin(GL_QUADS);
  glColor4f(0.1f, 0.05f, 0.1f, 0.3f);
  glVertex2f(-1.0f, -1.0f);
  glVertex2f(1.0f, -1.0f);
  glColor4f(0.1f, 0.05f, 0.1f, 0.0f);
  glVertex2f(1.0f, -0.5f);
  glVertex2f(-1.0f, -0.5f);
  glEnd();

  // Eerie glow on horizon
  glBegin(GL_QUADS);
  glColor4f(0.3f, 0.05f, 0.0f, 0.0f);
  glVertex2f(-1.0f, -0.2f);
  glVertex2f(1.0f, -0.2f);
  glColor4f(0.4f, 0.1f, 0.05f, 0.15f);
  glVertex2f(1.0f, 0.1f);
  glVertex2f(-1.0f, 0.1f);
  glEnd();

  glBegin(GL_QUADS);
  glColor4f(0.4f, 0.1f, 0.05f, 0.15f);
  glVertex2f(-1.0f, 0.1f);
  glVertex2f(1.0f, 0.1f);
  glColor4f(0.2f, 0.02f, 0.0f, 0.0f);
  glVertex2f(1.0f, 0.4f);
  glVertex2f(-1.0f, 0.4f);
  glEnd();

  // Draw some distant dead trees silhouettes
  glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
  for (int i = 0; i < 8; i++) {
    float tx = -0.9f + i * 0.25f;
    float th = 0.15f + (i % 3) * 0.08f;
    float ty = -0.6f + (i % 2) * 0.1f;
    // Tree trunk
    glBegin(GL_QUADS);
    glVertex2f(tx - 0.008f, ty);
    glVertex2f(tx + 0.008f, ty);
    glVertex2f(tx + 0.005f, ty + th);
    glVertex2f(tx - 0.005f, ty + th);
    glEnd();
    // Dead branches
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(tx, ty + th * 0.6f);
    glVertex2f(tx - 0.04f, ty + th * 0.8f);
    glVertex2f(tx, ty + th * 0.7f);
    glVertex2f(tx + 0.035f, ty + th * 0.9f);
    glVertex2f(tx, ty + th * 0.85f);
    glVertex2f(tx - 0.02f, ty + th + 0.03f);
    glEnd();
  }

  // Draw some stars/particles in the sky
  glPointSize(1.5f);
  glBegin(GL_POINTS);
  glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
  float starPositions[][2] = {{-0.8f, 0.7f},  {-0.5f, 0.85f}, {-0.3f, 0.6f},
                              {0.1f, 0.8f},   {0.4f, 0.75f},  {0.6f, 0.9f},
                              {0.75f, 0.65f}, {-0.6f, 0.55f}, {-0.2f, 0.9f},
                              {0.3f, 0.6f},   {0.5f, 0.5f},   {-0.7f, 0.45f}};
  for (int i = 0; i < 12; i++) {
    glVertex2f(starPositions[i][0], starPositions[i][1]);
  }
  glEnd();

  // Red moon
  glColor4f(0.6f, 0.1f, 0.1f, 0.5f);
  drawCircle(-0.7f, 0.75f, 0.08f);
  glColor4f(0.8f, 0.2f, 0.15f, 0.3f);
  drawCircle(-0.7f, 0.75f, 0.1f);
}

void drawZones() {
  drawSquare(serverLeft, -1.0f, serverRight - serverLeft, 2.0f, 0.3f, 1.0f,
             0.3f, 0.1f);

  drawSquare(hackerLeft, -1.0f, hackerRight - hackerLeft, 2.0f, 1.0f, 0.3f,
             0.3f, 0.1f);
}

void drawCircle(float cx, float cy, float r) {
  const int numSegments = 360;
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(cx, cy);
  for (int i = 0; i <= numSegments; i++) {
    float theta = 2.0f * M_PI * float(i) / float(numSegments);
    float x = r * cosf(theta);
    float y = r * sinf(theta);
    glVertex2f(cx + x, cy + y);
  }
  glEnd();
}

void drawHealthPickup(float x, float y) {
  glColor3f(0.9f, 0.1f, 0.1f);
  drawCircle(x, y, healthPickupRadius);
  glColor3f(1.0f, 1.0f, 1.0f);
  glLineWidth(2.0f);
  glBegin(GL_LINES);
  glVertex2f(x - 0.012f, y);
  glVertex2f(x + 0.012f, y);
  glVertex2f(x, y - 0.012f);
  glVertex2f(x, y + 0.012f);
  glEnd();
}

void drawFreezePickup(float x, float y) {
  glColor3f(0.2f, 0.6f, 1.0f);
  drawCircle(x, y, freezePickupRadius);
  glColor3f(1.0f, 1.0f, 1.0f);
  glLineWidth(2.0f);
  glBegin(GL_LINES);
  // Snowflake pattern
  glVertex2f(x - 0.015f, y);
  glVertex2f(x + 0.015f, y);
  glVertex2f(x, y - 0.015f);
  glVertex2f(x, y + 0.015f);
  glVertex2f(x - 0.01f, y - 0.01f);
  glVertex2f(x + 0.01f, y + 0.01f);
  glVertex2f(x - 0.01f, y + 0.01f);
  glVertex2f(x + 0.01f, y - 0.01f);
  glEnd();
}

void drawStickman() {
  // === SOLDIER CHARACTER ===

  // Helmet (military green)
  glColor3f(0.2f, 0.35f, 0.2f);
  drawCircle(0.0f, 0.85f, 0.12f);
  // Helmet rim
  drawSquare(-0.14f, 0.78f, 0.28f, 0.04f, 0.15f, 0.25f, 0.15f);

  // Face (skin tone)
  glColor3f(0.96f, 0.8f, 0.65f);
  drawCircle(0.0f, 0.75f, 0.08f);

  // Eyes (determined look)
  glColor3f(0.1f, 0.1f, 0.1f);
  drawSquare(-0.04f, 0.76f, 0.025f, 0.015f, 0.1f, 0.1f, 0.1f);
  drawSquare(0.015f, 0.76f, 0.025f, 0.015f, 0.1f, 0.1f, 0.1f);

  // Eyebrows (serious expression)
  glLineWidth(2.0f);
  glColor3f(0.3f, 0.2f, 0.1f);
  glBegin(GL_LINES);
  glVertex2f(-0.05f, 0.80f);
  glVertex2f(-0.02f, 0.79f);
  glVertex2f(0.02f, 0.79f);
  glVertex2f(0.05f, 0.80f);
  glEnd();

  // Mouth (slight grin)
  glLineWidth(2.0f);
  glColor3f(0.6f, 0.3f, 0.3f);
  glBegin(GL_LINE_STRIP);
  glVertex2f(-0.03f, 0.70f);
  glVertex2f(0.0f, 0.69f);
  glVertex2f(0.03f, 0.70f);
  glEnd();

  // Neck
  drawSquare(-0.03f, 0.62f, 0.06f, 0.05f, 0.96f, 0.8f, 0.65f);

  // Body armor (tactical vest)
  glColor3f(0.25f, 0.25f, 0.25f);
  drawSquare(-0.12f, 0.30f, 0.24f, 0.32f, 0.25f, 0.25f, 0.25f);
  // Vest details - pockets
  glColor3f(0.2f, 0.2f, 0.2f);
  drawSquare(-0.10f, 0.45f, 0.08f, 0.08f, 0.2f, 0.2f, 0.2f);
  drawSquare(0.02f, 0.45f, 0.08f, 0.08f, 0.2f, 0.2f, 0.2f);
  // Belt
  glColor3f(0.4f, 0.3f, 0.2f);
  drawSquare(-0.12f, 0.28f, 0.24f, 0.04f, 0.4f, 0.3f, 0.2f);
  // Belt buckle
  glColor3f(0.8f, 0.7f, 0.3f);
  drawSquare(-0.02f, 0.285f, 0.04f, 0.03f, 0.8f, 0.7f, 0.3f);

  // Shoulders (armor pads)
  glColor3f(0.3f, 0.3f, 0.3f);
  drawSquare(-0.18f, 0.52f, 0.08f, 0.08f, 0.3f, 0.3f, 0.3f);
  drawSquare(0.10f, 0.52f, 0.08f, 0.08f, 0.3f, 0.3f, 0.3f);

  // Arms (skin with sleeve)
  // Left arm
  glColor3f(0.2f, 0.35f, 0.2f); // Sleeve
  drawSquare(-0.22f, 0.38f, 0.06f, 0.16f, 0.2f, 0.35f, 0.2f);
  glColor3f(0.96f, 0.8f, 0.65f); // Hand
  drawCircle(-0.19f, 0.35f, 0.04f);

  // Right arm (holding gun forward)
  glColor3f(0.2f, 0.35f, 0.2f); // Sleeve
  drawSquare(0.16f, 0.38f, 0.06f, 0.16f, 0.2f, 0.35f, 0.2f);
  glColor3f(0.96f, 0.8f, 0.65f); // Hand
  drawCircle(0.19f, 0.35f, 0.04f);

  // Gun in hand
  glColor3f(0.15f, 0.15f, 0.15f);
  drawSquare(-0.35f, 0.32f, 0.18f, 0.06f, 0.15f, 0.15f, 0.15f); // Barrel
  drawSquare(-0.20f, 0.28f, 0.08f, 0.12f, 0.2f, 0.2f, 0.2f);    // Handle
  // Gun details
  glColor3f(0.3f, 0.3f, 0.3f);
  drawSquare(-0.32f, 0.33f, 0.04f, 0.04f, 0.3f, 0.3f, 0.3f); // Sight

  // Legs (cargo pants)
  glColor3f(0.2f, 0.35f, 0.2f);
  drawSquare(-0.10f, 0.05f, 0.08f, 0.23f, 0.2f, 0.35f, 0.2f); // Left leg
  drawSquare(0.02f, 0.05f, 0.08f, 0.23f, 0.2f, 0.35f, 0.2f);  // Right leg
  // Knee pads
  glColor3f(0.15f, 0.15f, 0.15f);
  drawSquare(-0.10f, 0.12f, 0.08f, 0.05f, 0.15f, 0.15f, 0.15f);
  drawSquare(0.02f, 0.12f, 0.08f, 0.05f, 0.15f, 0.15f, 0.15f);

  // Boots
  glColor3f(0.1f, 0.08f, 0.05f);
  drawSquare(-0.11f, -0.02f, 0.09f, 0.07f, 0.1f, 0.08f, 0.05f); // Left boot
  drawSquare(0.02f, -0.02f, 0.09f, 0.07f, 0.1f, 0.08f, 0.05f);  // Right boot
}

void drawAssistant() {
  // Futuristic Drone Helper Design

  // Hovering oscillation effect
  float hover = sin(glutGet(GLUT_ELAPSED_TIME) / 200.0f) * 0.02f;
  glTranslatef(0.0f, hover, 0.0f);

  // Thruster Glow (Cyan/Blue energy field)
  glColor4f(0.0f, 0.8f, 1.0f, 0.4f);
  drawCircle(0.0f, 0.0f, 0.14f);

  // Main Chassis (Dark Sci-Fi Metal)
  glColor3f(0.2f, 0.25f, 0.3f);
  drawCircle(0.0f, 0.0f, 0.09f);

  // Central Core (Glowing Power Source)
  glColor3f(0.0f, 0.9f, 1.0f); // Bright Cyan
  drawCircle(0.0f, 0.0f, 0.04f);

  // Tactical Eye/Visor (Menacing Red)
  glColor3f(1.0f, 0.0f, 0.2f);
  drawSquare(-0.06f, 0.03f, 0.05f, 0.02f, 1.0f, 0.0f, 0.2f); // Visor slit

  // Weapon System (Dual Cannons pointing Left towards zombies)
  glColor3f(0.4f, 0.4f, 0.45f);
  // Top Cannon
  drawSquare(-0.18f, 0.02f, 0.12f, 0.03f, 0.4f, 0.4f, 0.45f);
  // Bottom Cannon
  drawSquare(-0.18f, -0.05f, 0.12f, 0.03f, 0.4f, 0.4f, 0.45f);

  // Engine Exhaust (Pulsing)
  glColor4f(1.0f, 0.6f, 0.2f, 0.8f);
  glBegin(GL_TRIANGLES);
  glVertex2f(-0.04f, -0.08f);
  glVertex2f(0.04f, -0.08f);
  glVertex2f(0.0f, -0.18f - (hover * 0.5f)); // Exhaust stretches with movement
  glEnd();
}

void drawShop() {
  // Background
  drawSquare(-0.7f, -0.8f, 1.4f, 1.6f, 0.1f, 0.1f, 0.1f, 0.95f);
  renderText(-0.15f, 0.7f, "SHOP");
  // Bullets category
  renderText(-0.6f, 0.5f, "Bullets:");
  // Normal bullet
  drawBulletShopIcon(-0.55f, 0.4f, 1.0f, 1.0f, 1.0f);
  renderText(-0.45f, 0.42f, "Normal: Basic bullet (Free)");
  // Orange bullet
  drawBulletShopIcon(-0.55f, 0.25f, 1.0f, 0.5f, 0.0f);
  renderText(-0.45f, 0.27f, "Orange: Stronger bullet (10 coins)");
  // Red bullet
  drawBulletShopIcon(-0.55f, 0.1f, 1.0f, 0.0f, 0.0f);
  renderText(-0.45f, 0.12f, "Red: Strongest bullet (30 coins)");
  // Bullet upgrade hint
  renderText(-0.45f, -0.02f, "Press B to upgrade bullet");
  // Shield category
  renderText(-0.6f, -0.15f, "Shield:");
  drawShieldShopIcon(-0.55f, -0.25f);
  renderText(-0.45f, -0.23f, "Upgrade server shield (20 coins)");
  renderText(-0.45f, -0.33f, "Press U to upgrade shield");
  // Assistant category
  renderText(-0.6f, -0.45f, "Assistant:");
  glPushMatrix();
  glTranslatef(-0.55f, -0.55f, 0.0f);
  glScalef(0.4f, 0.4f, 1.0f); // Scale down for shop icon
  drawAssistant();
  glPopMatrix();

  renderText(-0.45f, -0.55f,
             "Hire helper drone (" + std::to_string(assistantCost) + " coins)");
  renderText(-0.45f, -0.65f,
             "Press A to hire (Owned: " + std::to_string(assistants.size()) +
                 ")");

  // Close shop hint
  renderText(-0.2f, -0.75f, "Press S to close shop");
}

void loadZombies(int count) {
  zombies.clear();
  healthPickups.clear();
  freezePickups.clear();
  for (int i = 0; i < count; ++i) {
    Zombie z;
    z.x = -1.0f - i * 0.3f;
    z.baseY = ((rand() % 180) - 90) / 100.0f;
    z.y = z.baseY;
    z.angle = 0.0f;
    z.zigzagEnabled = (currentDifficulty == DIFFICULTY_HARD) || (level > 5);
    z.isAlive = true;
    if (level >= 10) {
      z.health = 5;
    } else if (level >= 4) {
      z.health = 4;
    } else if (level >= 3) {
      z.health = 3;
    } else if (level >= 2) {
      z.health = 2;
    } else {
      z.health = 1;
    }
    zombies.push_back(z);
  }
}

void drawGameOverMenu() {
  drawSquare(-0.6f, -0.4f, 1.2f, 0.8f, 0.05f, 0.05f, 0.05f, 0.9f);
  if (level == maxLevel) {
    renderText(-0.18f, 0.2f, "YOU WIN");
  } else {
    renderText(-0.22f, 0.2f, "GAME OVER");
  }
  renderText(-0.35f, 0.0f, "1 - Restart / Replay");
  renderText(-0.35f, -0.15f, "2 - Exit Game");
}

void drawDifficultyScreen() {
  drawSquare(-0.7f, -0.6f, 1.4f, 1.2f, 0.05f, 0.05f, 0.05f, 0.92f);
  renderText(-0.2f, 0.35f, "SELECT DIFFICULTY");
  renderText(-0.45f, 0.1f, "E - Easy (slow zombies, low damage)");
  renderText(-0.45f, -0.05f, "M - Medium (balanced speed and damage)");
  renderText(-0.45f, -0.2f, "H - Hard (fast zigzag, high damage)");
}

void resetGame() {
  zombies.clear();
  bullets.clear();
  healthPickups.clear();
  freezePickups.clear();
  zombiesFrozen = false;
  freezeEndTime = 0.0f;
  playerY = 0.0f;
  level = 1;
  zombieBaseSpeed = 0.002f;
  zombiesKilled = 0;
  gameOver = false;
  points = 0;
  serverUpgraded = false;
  zombieEntryTimes.clear();
  upgradeEndTime = 0.0f;
  bulletUpgradeLevel = 0;
  shopOpen = false;
  levelCountdownActive = false;
  levelCountdownValue = 5;
  currentAmmo = maxAmmo;
  reloading = false;
  playerHealth = playerMaxHealth;
  assistants.clear();
  difficultySelectionActive = true;
  loadZombies(10);
  glutPostRedisplay();
  glutTimerFunc(16, update, 0);
}

void display() {
  glClear(GL_COLOR_BUFFER_BIT);
  drawBackground();
  drawZones();
  renderText(0.7f, 0.9f, "Points: " + std::to_string(points));
  renderText(-0.95f, 0.85f,
             "Health: " + std::to_string(playerHealth) + "/" +
                 std::to_string(playerMaxHealth));
  std::string difficultyLabel = "Medium";
  if (currentDifficulty == DIFFICULTY_EASY) {
    difficultyLabel = "Easy";
  } else if (currentDifficulty == DIFFICULTY_HARD) {
    difficultyLabel = "Hard";
  }
  renderText(-0.95f, 0.78f, "Difficulty: " + difficultyLabel);
  if (zombiesFrozen) {
    float freezeRemaining =
        freezeEndTime - (glutGet(GLUT_ELAPSED_TIME) / 1000.0f);
    if (freezeRemaining > 0) {
      glColor3f(0.2f, 0.6f, 1.0f);
      renderText(-0.2f, 0.65f,
                 "FROZEN: " + std::to_string((int)freezeRemaining + 1) + "s");
    }
  }
  if (upgradeEndTime > 0.0f) {
    float remaining = upgradeEndTime - (glutGet(GLUT_ELAPSED_TIME) / 1000.0f);
    std::string timerText = "Upgrade: " + std::to_string((int)remaining) + "s";
    renderText(-0.2f, 0.8f, timerText);
  }
  if (upgradeEndTime > 0.0f) {
    drawSquare(serverLeft, -1.0f, serverRight - serverLeft, 2.0f, 0.2f, 0.2f,
               1.0f, 0.5f);
  } else {
    drawSquare(serverLeft, -1.0f, serverRight - serverLeft, 2.0f, 0.0f, 0.3f,
               0.0f, 0.1f);
  }
  if (bulletUpgradeLevel < 2) {
    std::string bulletUpgradeText;
    if (bulletUpgradeLevel == 0 && points >= orangeUpgradeCost) {
      bulletUpgradeText = "Press B for Orange Bullets (10)";
    } else if (bulletUpgradeLevel == 1 && points >= redUpgradeCost) {
      bulletUpgradeText = "Press B for Red Bullets (30)";
    }

    if (!bulletUpgradeText.empty()) {
      renderText(-0.45f, 0.75f, bulletUpgradeText);
    }
  }

  if (gameOver) {
    drawGameOverMenu();
    glutSwapBuffers();
    return;
  }

  if (difficultySelectionActive) {
    drawDifficultyScreen();
    glutSwapBuffers();
    return;
  } else {
    if (points >= upgradeCost && upgradeEndTime <= 0.0f) {
      renderText(-0.45f, 0.85f,
                 "UPGRADE AVAILABLE! Press U to strengthen server (20 coins)");
    }
  }

  // Draw stickman player
  glPushMatrix();
  glTranslatef(0.95f, playerY, 0.0f);
  glScalef(0.2f, 0.2f, 1.0f);
  drawStickman();
  glPopMatrix();

  // Draw assistants
  for (const auto &assistant : assistants) {
    if (assistant.active) {
      glPushMatrix();
      glTranslatef(0.8f, assistant.y, 0.0f);
      glScalef(0.3f, 0.3f, 1.0f); // Doubled size (0.15 -> 0.3)
      drawAssistant();
      glPopMatrix();
    }
  }

  // Draw zombies
  for (const auto &zombie : zombies) {
    if (zombie.isAlive) {
      glPushMatrix();
      glTranslatef(zombie.x, zombie.y, 0.0f);
      glScalef(0.2f, 0.2f, 1.0f);
      if (zombiesFrozen) {
        drawFrozenZombie();
      } else {
        drawZombie();
      }
      glPopMatrix();
    }
  }

  // Draw bullets
  for (const auto &bullet : bullets) {
    if (bullet.active) {
      drawBullet(bullet.x, bullet.y);
    }
  }

  // Draw health pickups
  for (const auto &pickup : healthPickups) {
    if (pickup.active) {
      drawHealthPickup(pickup.x, pickup.y);
    }
  }

  // Draw freeze pickups
  for (const auto &pickup : freezePickups) {
    if (pickup.active) {
      drawFreezePickup(pickup.x, pickup.y);
    }
  }

  renderText(-0.9f, 0.9f, "Level: " + std::to_string(level));
  if (shopOpen) {
    drawShop();
    glutSwapBuffers();
    return;
  }

  drawAmmoBar();

  if (levelCountdownActive) {
    drawLevelCountdown();
  }

  glutSwapBuffers();
}

void purchaseBulletUpgrade() {
  if (bulletUpgradeLevel == 0 && points >= orangeUpgradeCost) {
    points -= orangeUpgradeCost;
    bulletUpgradeLevel = 1;
  } else if (bulletUpgradeLevel == 1 && points >= redUpgradeCost) {
    points -= redUpgradeCost;
    bulletUpgradeLevel = 2;
  }
}

void purchaseAssistant() {
  if (points >= assistantCost) {
    points -= assistantCost;
    Assistant newAssistant;
    newAssistant.active = true;
    newAssistant.y = 0.3f + (assistants.size() * 0.1f); // Offset spawn slightly
    // Keep spawn within bounds
    if (newAssistant.y > 0.8f)
      newAssistant.y = 0.8f;
    newAssistant.lastShotTime = 0.0f;
    assistants.push_back(newAssistant);
  }
}

void update(int value) {
  if (gameOver || shopOpen || difficultySelectionActive ||
      levelCountdownActive) // Pause game updates if shop is open or countdown
                            // is active
    return;

  if (keyUpPressed)
    playerY += 0.015f;
  if (keyDownPressed)
    playerY -= 0.015f;
  if (playerY > 0.9f)
    playerY = 0.9f;
  if (playerY < -0.95f)
    playerY = -0.95f;

  float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
  if (currentTime > upgradeEndTime) {
    upgradeEndTime = 0.0f;
  }

  // Assistant AI Logic (Distributed Targeting & Smooth Movement)
  if (!assistants.empty()) {
    // 1. Gather active targets
    std::vector<Zombie *> targets;
    for (auto &z : zombies) {
      if (z.isAlive && z.x < 0.8f) {
        targets.push_back(&z);
      }
    }

    // 2. Sort targets by Y (Top to Bottom) - Simple Bubble Sort
    if (!targets.empty()) {
      for (size_t j = 0; j < targets.size() - 1; ++j) {
        for (size_t k = 0; k < targets.size() - j - 1; ++k) {
          if (targets[k]->y < targets[k + 1]->y) {
            Zombie *temp = targets[k];
            targets[k] = targets[k + 1];
            targets[k + 1] = temp;
          }
        }
      }
    }

    // 3. Process each assistant
    for (size_t i = 0; i < assistants.size(); ++i) {
      auto &assistant = assistants[i];
      if (!assistant.active)
        continue;

      Zombie *targetZombie = nullptr;
      if (!targets.empty()) {
        // Distribute targets: Assistant 0 takes top, Assistant N takes bottom
        size_t targetIdx = (i * targets.size()) / assistants.size();
        targetZombie = targets[targetIdx];
      }

      // Movement
      if (targetZombie != nullptr) {
        float targetY = targetZombie->y;
        if (assistant.y < targetY - 0.02f)
          assistant.y += 0.015f;
        else if (assistant.y > targetY + 0.02f)
          assistant.y -= 0.015f;
      } else {
        // Idle: Spread out vertically if no targets
        // Map index 0..N to range 0.6..-0.6
        float idleY =
            0.6f -
            (1.2f * i / (assistants.size() > 1 ? assistants.size() - 1 : 1));
        if (assistants.size() == 1)
          idleY = 0.0f; // Center if only one

        if (assistant.y < idleY - 0.02f)
          assistant.y += 0.01f;
        else if (assistant.y > idleY + 0.02f)
          assistant.y -= 0.01f;
      }

      // Clamp
      if (assistant.y > 0.9f)
        assistant.y = 0.9f;
      if (assistant.y < -0.95f)
        assistant.y = -0.95f;

      // Shooting
      if (currentTime - assistant.lastShotTime >= assistantShotInterval) {
        if (targetZombie != nullptr &&
            abs(assistant.y - targetZombie->y) < 0.3f) {
          Bullet b;
          b.x = 0.8f - 0.1f;
          b.y = assistant.y;
          b.active = true;
          // Damage Buff: 50% more (approx double since int) after level 10
          b.damage = (level > 10) ? 2 : 1;
          b.r = 0.0f;
          b.g = 1.0f;
          b.b = 1.0f;
          bullets.push_back(b);
          assistant.lastShotTime = currentTime;
        }
      }
    }
  }

  if (level > 1) {
    zombieBaseSpeed = 0.002f + (level - 1) * 0.0005f;
  }
  for (auto &zombie : zombies) {
    if (zombie.isAlive) {
      // Skip movement if zombies are frozen
      if (!zombiesFrozen) {
        // Movement logic
        float horizontalSpeed = zombieBaseSpeed * difficultySpeedMultiplier;
        if (zombie.x < hackerRight) {
          horizontalSpeed += zombieBaseSpeed * 3 * difficultySpeedMultiplier;
        } else {
          horizontalSpeed += zombieBaseSpeed * difficultySpeedMultiplier;
        }
        if (zombie.zigzagEnabled) {
          zombie.angle += 0.1f;
          zombie.y = zombie.baseY + sin(zombie.angle) * 0.15f;
          horizontalSpeed *= 1.2f;
        }
        zombie.x += horizontalSpeed;
      }

      // Game over condition - zombie reaches server area and damages player
      if (zombie.x >= serverLeft) {
        int damage = zombieDamage;
        // Shield prevents all damage when active
        if (upgradeEndTime > 0.0f) {
          damage = 0;
        }
        playerHealth -= damage;
        if (damage > 0) {
          playSound(hitSound); // Player got hit
        }
        if (playerHealth <= 0) {
          playerHealth = 0;
          gameOver = true;
          playSound(gameOverSound); // Game over sound
        }
        checkServerBreach();
        zombie.isAlive = false;
      }
    }
  }
  for (auto &bullet : bullets) {
    if (bullet.active) {
      bullet.x -= 0.02f;

      // Check collisions
      for (auto &zombie : zombies) {
        if (zombie.isAlive) {
          float zxLeft = zombie.x - 0.12f * 0.2f;
          float zxRight = zombie.x + 0.12f * 0.2f;
          float zyTop = zombie.y + (0.15f + 0.24f) * 0.2f;
          float zyBottom = zombie.y - 0.3f * 0.2f;

          float bx = bullet.x + 0.025f;
          float by = bullet.y + 0.025f;

          if (bullet.x >= zombie.x - 0.024f && bullet.x <= zombie.x + 0.024f &&
              bullet.y >= zombie.y - 0.15f && bullet.y <= zombie.y + 0.15f) {
            bullet.active = false;
            zombie.health -= bullet.damage;
            if (zombie.health <= 0) {
              zombie.isAlive = false;
              playSound(zombieDeathSound); // Zombie killed
              zombiesKilled++;
              points += bullet.damage;

              // Only drop items if player health is less than max (100)
              if (playerHealth < 100) {
                float roll = static_cast<float>(rand()) / RAND_MAX;
                if (roll <= healthPickupDropChance) {
                  HealthPickup pickup;
                  pickup.x = zombie.x;
                  pickup.y = zombie.y;
                  pickup.active = true;

                  // Health increase logic based on level
                  if (level == 1) {
                    pickup.healAmount = 3;
                  } else if (level == 2) {
                    pickup.healAmount = 6;
                  } else {
                    pickup.healAmount = 10;
                  }

                  pickup.radius = healthPickupRadius;
                  healthPickups.push_back(pickup);
                } else if (roll <=
                           healthPickupDropChance + freezePickupDropChance) {
                  FreezePickup fPickup;
                  fPickup.x = zombie.x;
                  fPickup.y = zombie.y;
                  fPickup.active = true;
                  fPickup.radius = freezePickupRadius;
                  freezePickups.push_back(fPickup);
                }
              }
            }
            break;
          }
        }
      }

      // Remove off-screen bullets
      if (bullet.x < -1.1f)
        bullet.active = false;
    }
  }
  for (auto &pickup : healthPickups) {
    if (pickup.active) {
      // Move pickup toward player
      float dx = 0.95f - pickup.x;
      float dy = playerY - pickup.y;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > 0.01f) {
        float speed = 0.015f;
        pickup.x += (dx / distance) * speed;
        pickup.y += (dy / distance) * speed;
      }
      // Check if player collects pickup
      if (distance <= pickup.radius + 0.1f) {
        playerHealth += pickup.healAmount;
        if (playerHealth > playerMaxHealth) {
          playerHealth = playerMaxHealth;
        }
        playSound(pickupSound); // Health pickup sound
        pickup.active = false;
      }
    }
  }

  // Move and collect freeze pickups
  for (auto &pickup : freezePickups) {
    if (pickup.active) {
      float dx = 0.95f - pickup.x;
      float dy = playerY - pickup.y;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > 0.01f) {
        float speed = 0.015f;
        pickup.x += (dx / distance) * speed;
        pickup.y += (dy / distance) * speed;
      }
      if (distance <= pickup.radius + 0.1f) {
        zombiesFrozen = true;
        freezeEndTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f + freezeDuration;
        playSound(freezeSound); // Freeze pickup sound
        pickup.active = false;
      }
    }
  }

  // Check if freeze expired
  if (zombiesFrozen && currentTime >= freezeEndTime) {
    zombiesFrozen = false;
  }

  bool allDead = true;
  for (const auto &zombie : zombies) {
    if (zombie.isAlive) {
      allDead = false;
      break;
    }
  }
  if (allDead) {
    if (level < maxLevel) {
      level++;
      playSound(levelUpSound); // Level up sound
      zombiesKilled = 0;
      startLevelCountdown(); // Start countdown before loading new level
    } else {
      gameOver = true;
      playSound(gameOverSound); // Win sound (game over)
    }
  }
  glutPostRedisplay();
  glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
  if (gameOver) {
    if (key == '1' || key == 'r' || key == 'R') {
      resetGame();
    } else if (key == '2' || key == 'e' || key == 'E' || key == 27) {
      exit(0);
    }
    return;
  }
  if (difficultySelectionActive) {
    if (key == 'e' || key == 'E') {
      currentDifficulty = DIFFICULTY_EASY;
      difficultySpeedMultiplier = 0.75f;
      zombieDamage = 14;
    } else if (key == 'm' || key == 'M') {
      currentDifficulty = DIFFICULTY_MEDIUM;
      difficultySpeedMultiplier = 1.0f;
      zombieDamage = 28;
    } else if (key == 'h' || key == 'H') {
      currentDifficulty = DIFFICULTY_HARD;
      difficultySpeedMultiplier = 1.15f;
      zombieDamage = 56;
    } else {
      return;
    }
    difficultySelectionActive = false;
    loadZombies(6 + level);
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
    return;
  }
  if (key == 32) {
    if (currentAmmo > 0) {
      Bullet b;
      b.x = 0.95f;
      b.y = playerY;
      b.active = true;
      switch (bulletUpgradeLevel) {
      case 2: // Red bullets
        b.damage = 3;
        b.r = 1.0f;
        b.g = 0.0f;
        b.b = 0.0f;
        break;
      case 1: // Orange bullets
        b.damage = 2;
        b.r = 1.0f;
        b.g = 0.5f;
        b.b = 0.0f;
        break;
      default: // Normal bullets
        b.damage = 1;
        b.r = 1.0f;
        b.g = 1.0f;
        b.b = 1.0f;
      }
      bullets.push_back(b);
      playSound(shootSound); // Play shooting sound
      currentAmmo--;
      if (!reloading) {
        reloading = true;
        glutTimerFunc(200, reloadAmmo, 0);
      }
    }
  } else if (shopOpen) {
    if (key == 's' || key == 'S') {
      shopOpen = false;
      glutPostRedisplay();
      glutTimerFunc(16, update, 0); // Resume game updates
      return;
    }
    if (key == 'b' || key == 'B') {
      purchaseBulletUpgrade();
      glutPostRedisplay();
      return;
    }
    if (key == 'u' || key == 'U') {
      purchaseUpgrade();
      glutPostRedisplay();
      return;
    }
    if (key == 'a' || key == 'A') {
      purchaseAssistant();
      glutPostRedisplay();
      return;
    }
    return;
  }
  if (key == 's' || key == 'S') {
    shopOpen = true;
    glutPostRedisplay();
    return;
  } else if (key == 'u' || key == 'U' && !gameOver) {
    purchaseUpgrade();
  } else if (key == 27) {
    exit(0);
  } else if (key == 'b' || key == 'B') {
    purchaseBulletUpgrade();
  }
}

void specialDown(int key, int x, int y) {
  if (key == GLUT_KEY_UP)
    keyUpPressed = true;
  else if (key == GLUT_KEY_DOWN)
    keyDownPressed = true;
}

void specialUp(int key, int x, int y) {
  if (key == GLUT_KEY_UP)
    keyUpPressed = false;
  else if (key == GLUT_KEY_DOWN)
    keyDownPressed = false;
}

void initSound() {
  // Initialize SDL audio
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("SDL audio init failed: %s\n", SDL_GetError());
    soundEnabled = false;
    return;
  }

  // Initialize SDL_mixer
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("SDL_mixer init failed: %s\n", Mix_GetError());
    soundEnabled = false;
    return;
  }

  // Load sound effects (shoot.wav has no mp3 version, use wav on all platforms)
  shootSound = Mix_LoadWAV("sounds/shoot.wav");
  if (!shootSound) {
    printf("Warning: Could not load shoot sound: %s\n", Mix_GetError());
  } else {
    Mix_VolumeChunk(shootSound, 24); // Set to 24 (20% higher than 20)
  }

#ifdef __EMSCRIPTEN__
  zombieDeathSound = Mix_LoadWAV("sounds/zombie_death.mp3");
#else
  zombieDeathSound = Mix_LoadWAV("sounds/zombie_death.wav");
#endif
  if (!zombieDeathSound) {
    printf("Warning: Could not load zombie_death sound: %s\n", Mix_GetError());
  }

#ifdef __EMSCRIPTEN__
  pickupSound = Mix_LoadWAV("sounds/pickup.mp3");
#else
  pickupSound = Mix_LoadWAV("sounds/pickup.wav");
#endif
  if (!pickupSound) {
    printf("Warning: Could not load pickup sound: %s\n", Mix_GetError());
  }

#ifdef __EMSCRIPTEN__
  freezeSound = Mix_LoadWAV("sounds/freeze.mp3");
#else
  freezeSound = Mix_LoadWAV("sounds/freeze.wav");
#endif
  if (!freezeSound) {
    printf("Warning: Could not load freeze sound: %s\n", Mix_GetError());
  }

#ifdef __EMSCRIPTEN__
  levelUpSound = Mix_LoadWAV("sounds/level_up.mp3");
#else
  levelUpSound = Mix_LoadWAV("sounds/level_up.wav");
#endif
  if (!levelUpSound) {
    printf("Warning: Could not load level_up sound: %s\n", Mix_GetError());
  }

  // Load background music
#ifdef __EMSCRIPTEN__
  backgroundMusic = Mix_LoadMUS("sounds/background_sound_track.mp3");
#else
  backgroundMusic = Mix_LoadMUS("sounds/background_sound_track.wav");
#endif
  if (!backgroundMusic) {
    printf("Warning: Could not load background music: %s\n",
           Mix_GetError());
  }

  // Set volume (0-128)
  Mix_Volume(-1, 64);  // All channels
  Mix_VolumeMusic(32); // Background music quieter

  // Play background music on loop
  if (backgroundMusic) {
    Mix_PlayMusic(backgroundMusic, -1);
  }
}

void cleanupSound() {
  if (shootSound)
    Mix_FreeChunk(shootSound);
  if (zombieDeathSound)
    Mix_FreeChunk(zombieDeathSound);
  if (pickupSound)
    Mix_FreeChunk(pickupSound);
  if (freezeSound)
    Mix_FreeChunk(freezeSound);
  if (gameOverSound)
    Mix_FreeChunk(gameOverSound);
  if (levelUpSound)
    Mix_FreeChunk(levelUpSound);
  if (hitSound)
    Mix_FreeChunk(hitSound);
  if (backgroundMusic)
    Mix_FreeMusic(backgroundMusic);

  Mix_CloseAudio();
  SDL_Quit();
}

void playSound(Mix_Chunk *sound) {
  if (soundEnabled && sound) {
    if (Mix_PlayChannel(-1, sound, 0) == -1) {
      printf("Mix_PlayChannel error: %s\n", Mix_GetError());
    }
  }
}

void init() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(-1.0f, 1.0f, -1.0f, 1.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  srand(static_cast<unsigned>(time(0)));

  // Initialize sound system
  initSound();

  loadZombies(10);
}

void reshape(int width, int height) {
  if (height == 0) {
    height = 1;
  }

  float targetAspect =
      static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
  int viewportWidth = width;
  int viewportHeight = static_cast<int>(width / targetAspect);

  if (viewportHeight > height) {
    viewportHeight = height;
    viewportWidth = static_cast<int>(height * targetAspect);
  }

  int viewportX = (width - viewportWidth) / 2;
  int viewportY = (height - viewportHeight) / 2;

  glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-1.0f, 1.0f, -1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void drawRocketBullet(float x, float y, float r, float g, float b) {
  // Body of the rocket
  glColor3f(r, g, b);
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(x, y);                // Bottom-left
  glVertex2f(x + 0.05f, y);        // Bottom-right
  glVertex2f(x + 0.05f, y + 0.2f); // Top-right
  glVertex2f(x, y + 0.2f);         // Top-left
  glEnd();

  // Tip of the rocket
  glColor3f(1.0f, 0.0f, 0.0f); // Red tip
  glBegin(GL_TRIANGLES);
  glVertex2f(x, y + 0.2f);           // Base-left
  glVertex2f(x + 0.05f, y + 0.2f);   // Base-right
  glVertex2f(x + 0.025f, y + 0.25f); // Tip
  glEnd();

  // Fins of the rocket
  glColor3f(0.5f, 0.5f, 0.5f); // Gray fins
  glBegin(GL_TRIANGLES);
  glVertex2f(x, y);                 // Bottom-left
  glVertex2f(x - 0.02f, y + 0.05f); // Left fin
  glVertex2f(x, y + 0.05f);         // Top-left fin

  glVertex2f(x + 0.05f, y);         // Bottom-right
  glVertex2f(x + 0.07f, y + 0.05f); // Right fin
  glVertex2f(x + 0.05f, y + 0.05f); // Top-right fin
  glEnd();
}

#ifdef __EMSCRIPTEN__
void main_loop() { display(); }
#endif

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(screenWidth, screenHeight);
  glutCreateWindow("Zombie Shooter");

#ifndef __EMSCRIPTEN__
  glutFullScreen();
#endif

  init();
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(specialDown);
  glutSpecialUpFunc(specialUp);
  glutTimerFunc(25, update, 0);

  // Register cleanup on exit
  atexit(cleanupSound);

#ifdef __EMSCRIPTEN__
  emscripten_set_canvas_element_size("#canvas", screenWidth, screenHeight);
  reshape(screenWidth, screenHeight);
  glutReshapeFunc(reshape);
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  glutMainLoop();
#endif

  return 0;
}
