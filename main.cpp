/*
    Project: Catch The Eggs - Deluxe Edition
    Creators: Momitu & Labony
    Course: CSE 426 - Computer Graphics Lab
*/

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

#include <GL/glut.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#pragma comment(lib, "winmm.lib")

// ==========================================
// 1. GLOBAL SETTINGS & STATE
// ==========================================
enum State { MENU, HELP, PLAYING, GAMEOVER };
State currentState = MENU;

float basketX = 0.0f;
float basketWidth = 22.0f;
int score = 0, highScore = 0, lives = 3, remainingTime = 120;

bool isPaused = false;

// Physics Variables
float eggSpeed = 1.6f;
float originalSpeed = 1.6f;
float airflow = 0.0f;
int perkEffectTimer = 0;

// Chicken Movement
float chX[4] = {-60, 20, -30, 50};
float chDir[4] = {0.5, -0.7, 0.9, -0.4};

struct Egg {
    float x, y;
    int type; // 0:White, 1:Blue, 2:Gold, 3:Shit, 4:Big, 5:Slow, 6:Time, 7:Shrink
    bool active, broken;
    int brokenTimer;
} gameEgg;

// ==========================================
// 2. SOUND & FILE SYSTEM
// ==========================================
void playBGM() {
    mciSendString("close all", NULL, 0, NULL);
    mciSendString("open \"bg_music.wav\" type waveaudio alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
}

void playSFX(std::string file) {
    // PlaySound with SND_NOSTOP prevents interrupting the BGM
    PlaySound(TEXT(file.c_str()), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}

void saveScore() {
    std::ofstream f("highscore.txt");
    if (f.is_open()) f << highScore;
    f.close();
}

void loadScore() {
    std::ifstream f("highscore.txt");
    if (f.is_open()) f >> highScore;
    f.close();
}

// ==========================================
// 3. GRAPHICS RENDERING
// ==========================================
void drawText(float x, float y, std::string str, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : str) glutBitmapCharacter(font, c);
}

void drawCircle(float x, float y, float rx, float ry) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 10) {
        float rad = i * 0.01745f;
        glVertex2f(x + cos(rad) * rx, y + sin(rad) * ry);
    }
    glEnd();
}

void drawHeart(float x, float y, float r) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 5) {
        float rad = i * 0.01745f;
        // Mathematical formula for a smooth 2D Heart shape
        float hx = 16 * sin(rad) * sin(rad) * sin(rad);
        float hy = 13 * cos(rad) - 5 * cos(2 * rad) - 2 * cos(3 * rad) - cos(4 * rad);

        // Scale down with radius 'r' and position at (x, y)
        glVertex2f(x + (hx * r * 0.18f), y + (hy * r * 0.18f));
    }
    glEnd();
}


void drawChicken(float x, float y) {
    // Body
    glColor3f(1, 1, 1); drawCircle(x, y, 10, 7);
    // Head
    drawCircle(x + 8, y + 5, 5, 5);
    // Eye
    glColor3f(0, 0, 0); drawCircle(x + 10, y + 6, 0.8, 0.8);
    // Beak
    glColor3f(1.0, 0.5, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex2f(x+13, y+5); glVertex2f(x+17, y+6); glVertex2f(x+17, y+4);
    glEnd();
    // Comb (The Red Crown)
    glColor3f(0.8, 0, 0);
    drawCircle(x + 8, y + 10, 2, 3);
}

void drawRealisticEgg(float x, float y, int type) {
    if (type == 3) { // POOP/SHIT (Distinct Brown Swirl)
        glColor3f(0.4, 0.2, 0.1);
        drawCircle(x, y - 2, 5, 3);
        drawCircle(x, y, 4, 2.5);
        drawCircle(x, y + 2, 2.5, 2);
    } else {
        // Color Mapping
        if (type == 0) glColor3f(1, 1, 1);
        else if (type == 1) glColor3f(0.2, 0.5, 1);
        else if (type == 2) glColor3f(1, 0.8, 0);
        else if (type == 4) glColor3f(0, 1, 1);
        else if (type == 5) glColor3f(1, 0, 1);
        else if (type == 6) glColor3f(0, 1, 0);
        else if (type == 7) glColor3f(0.8, 0, 0);

        // Egg Shape (Bottom Heavy)
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++) {
            float rad = i * 0.01745f;
            float rx = 3.2f, ry = 4.8f;
            if (i > 0 && i < 180) ry = 5.2f; // Make top slightly more tapered
            glVertex2f(x + cos(rad) * rx, y + sin(rad) * ry);
        }
        glEnd();
        // Shine highlight
        glColor4f(1, 1, 1, 0.5);
        drawCircle(x - 1.2, y + 2, 0.7, 1.2);
    }
}

void drawBasket(float x, float w) {
    // Body
    glColor3f(0.5, 0.3, 0.1);
    glRectf(x - w, -85, x + w, -68);
    // Rim
    glColor3f(0.4, 0.2, 0.0);
    glRectf(x - w - 2, -70, x + w + 2, -65);
    // Texture lines
    glColor3f(0.3, 0.1, 0.0);
    glBegin(GL_LINES);
    for (float i = x - w + 2; i < x + w; i += 5) { glVertex2f(i, -85); glVertex2f(i, -70); }
    glEnd();
}

// ==========================================
// 4. GAME ENGINE LOGIC
// ==========================================
void resetEgg() {
    int source = rand() % 4;
    gameEgg.x = chX[source];
    gameEgg.y = (source < 2) ? 78 : 48;

    int r = rand() % 100;
    if (r < 45) gameEgg.type = 0;
    else if (r < 55) gameEgg.type = 1;
    else if (r < 62) gameEgg.type = 2;
    else if (r < 75) gameEgg.type = 3; // Poop
    else if (r < 82) gameEgg.type = 4;
    else if (r < 88) gameEgg.type = 5;
    else if (r < 94) gameEgg.type = 6;
    else gameEgg.type = 7;

    gameEgg.active = true; gameEgg.broken = false; gameEgg.brokenTimer = 0;
    airflow = ((rand() % 21) - 10) / 30.0f;
}

void update(int v) {
    if (currentState == PLAYING && !isPaused) {
        for (int i = 0; i < 4; i++) {
            chX[i] += chDir[i];
            if (chX[i] > 85 || chX[i] < -85) chDir[i] *= -1;
        }

        if (!gameEgg.broken) {
            gameEgg.y -= eggSpeed;
            gameEgg.x += airflow;

            // Catch Logic
            if (gameEgg.y < -65 && gameEgg.y > -80 && abs(gameEgg.x - basketX) < basketWidth + 2) {
                playSFX("catch.wav");
                if (gameEgg.type == 0) score += 1;
                else if (gameEgg.type == 1) score += 5;
                else if (gameEgg.type == 2) score += 10;
                else if (gameEgg.type == 3) { score -= 10; if (score < 0) score = 0; }
                else if (gameEgg.type == 4) { basketWidth = 35; perkEffectTimer = 400; }
                else if (gameEgg.type == 5) { eggSpeed = 1.0; perkEffectTimer = 400; }
                else if (gameEgg.type == 6) remainingTime += 15;
                else if (gameEgg.type == 7) { basketWidth = 12; perkEffectTimer = 300; }

                eggSpeed += 0.02; originalSpeed += 0.01;
                resetEgg();
            }

            // Floor Break Logic
            if (gameEgg.y < -85) {
                gameEgg.broken = true;
                playSFX("break.wav");
                if (gameEgg.type <= 2) lives--;
            }
        } else {
            gameEgg.brokenTimer++;
            if (gameEgg.brokenTimer > 15) resetEgg();
        }

        if (perkEffectTimer > 0) {
            perkEffectTimer--;
            if (perkEffectTimer == 0) { basketWidth = 22; eggSpeed = originalSpeed; }
        }

        if (lives <= 0 || remainingTime <= 0) {
            currentState = GAMEOVER;
            if (score > highScore) { highScore = score; saveScore(); }
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void clockTick(int v) {
    if (currentState == PLAYING && !isPaused) remainingTime--;
    glutTimerFunc(1000, clockTick, 0);
}

// ==========================================
// 5. INTERFACE & MAIN
// ==========================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (currentState == MENU) {
        glClearColor(0.08, 0.1, 0.15, 1);
        glColor3f(1, 1, 0.5);
        drawText(-35, 40, "CATCH THE EGGS: DELUXE", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1, 1, 1);
        drawText(-22, 10, "Press 'S' to Start Game");
        drawText(-22, -5, "Press 'H' for Help");
        drawText(-22, -20, "Press 'Q' to Quit");
        glColor3f(0.5, 0.8, 1);
        drawText(-25, -60, "Made by - Momitu & Labony");
        if(highScore > 0) {
            std::stringstream hs; hs << "High Score: " << highScore;
            glColor3f(1, 0.8, 0); drawText(-15, -40, hs.str());
        }
    } else if (currentState == HELP) {
        glClearColor(0.1, 0.1, 0.1, 1);
        glColor3f(1, 1, 1);
        drawText(-30, 60, "--- HOW TO PLAY ---", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(-85, 30, "-> Catch White/Blue/Gold eggs for points.");
        drawText(-85, 15, "-> AVOID THE POOP (Brown). Lose 10 points!");
        drawText(-85, 0, "-> Missing any good egg costs 1 LIFE.");
        drawText(-85, -15, "-> Blue/Pink/Green eggs are special PERKS.");
        drawText(-20, -60, "Press 'B' to Go Back");
    } else {
        // Gameplay Background
        // Sky Color background
glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);

// Draw Green Grass Area at the bottom
glColor3f(0.2f, 0.8f, 0.2f);
glRectf(-100, -100, 100, -85);
        if(lives == 1) glClearColor(0.7, 0.3, 0.3, 1); // Danger Warning
        else glClearColor(0.5, 0.8, 1.0, 1);

        // Bamboo sticks
        glColor3f(0.3, 0.2, 0.1);
        glRectf(-100, 78, 100, 81); glRectf(-100, 48, 100, 51);

        for (int i = 0; i < 4; i++) drawChicken(chX[i], (i < 2 ? 86 : 56));

        // HUD
        glColor3f(0, 0, 0);
        std::stringstream ss, st;
        ss << "Score: " << score << "  Best: " << highScore; drawText(-95, 92, ss.str());
        st << "Time: " << remainingTime << "s"; drawText(70, 92, st.str());

// HUD Health display
        if (lives == 1) {
            glColor3f(1.0f, 0.0f, 0.0f); // Make text bright red when critical
        } else {
            glColor3f(0.0f, 0.0f, 0.0f); // Default black text
        }
        drawText(15, 92, "HEALTH:");

        // Draw Heart-Shaped Lives
        for(int i = 0; i < lives; i++) {
            glColor3f(0.9f, 0.1f, 0.1f); // Beautiful heart red color
            drawHeart(40.0f + (i * 8.0f), 93.5f, 1.2f);
        }

        drawBasket(basketX, basketWidth);
        if (gameEgg.active) drawRealisticEgg(gameEgg.x, gameEgg.y, gameEgg.type);
        if (gameEgg.broken) { glColor4f(1, 1, 1, 0.6); drawCircle(gameEgg.x, -88, 8, 2); }

        if (isPaused) drawText(-10, 0, "PAUSED", GLUT_BITMAP_TIMES_ROMAN_24);
        if (currentState == GAMEOVER) {
            glColor3f(0.8, 0, 0); drawText(-15, 10, "GAME OVER", GLUT_BITMAP_TIMES_ROMAN_24);
            glColor3f(0,0,0); drawText(-18, -5, "Press 'S' to Restart");
        }
    }
    glutSwapBuffers();
}

void keyboard(unsigned char k, int x, int y) {
    if (k == 's' || k == 'S') {
        if (currentState != PLAYING) {
            score = 0; lives = 3; remainingTime = 120;
            eggSpeed = 1.6; originalSpeed = 1.6;
            currentState = PLAYING;
            playBGM();
            resetEgg();
        }
    }
    if (k == 'b' || k == 'B') currentState = MENU;
    if (k == 'h' || k == 'H') currentState = HELP;
    if (k == 'p' || k == 'P') isPaused = !isPaused;
    if (k == 'q' || k == 'Q' || k == 27) exit(0);
}

void special(int k, int x, int y) {
    if (k == GLUT_KEY_LEFT && basketX > -80) basketX -= 10;
    if (k == GLUT_KEY_RIGHT && basketX < 80) basketX += 10;
}

void mouse(int x, int y) {
    basketX = (x / (float)glutGet(GLUT_WINDOW_WIDTH)) * 200.0f - 100.0f;
    if (basketX > 80) basketX = 80; if (basketX < -80) basketX = -80;
}

int main(int argc, char** argv) {
    srand(time(NULL));
    loadScore();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Catch The Eggs - HD Edition");
    gluOrtho2D(-100, 100, -100, 100);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutPassiveMotionFunc(mouse);
    glutTimerFunc(16, update, 0);
    glutTimerFunc(1000, clockTick, 0);
    glutMainLoop();
    return 0;
}
