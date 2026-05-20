/*
    Project: Catch The Eggs - Deluxe Edition
    Creators: Momitu & Labony
    Course: CSE 426 - Computer Graphics Lab
*/

#ifdef _WIN32
#include <windows.h>  // System Audio API
#include <mmsystem.h> // Multimedia Sound Library
#endif

#include <GL/glut.h>   // OpenGL ToolKit
#include <iostream>
#include <string>
#include <sstream>     // Number-to-String Buffer
#include <fstream>     // File Read/Write
#include <stdlib.h>
#include <time.h>
#include <math.h>

#pragma comment(lib, "winmm.lib") // Link Audio Hardware

// ==========================================
// 1. GLOBAL SETTINGS & STATE
// ==========================================
// Game Screen States
enum State { MENU, HELP, PLAYING, GAMEOVER };
State currentState = MENU;

float basketX = 0.0f;       // Catcher X-Position
float basketWidth = 22.0f;  // Collision Width Boundary
int score = 0, highScore = 0, lives = 3, remainingTime = 120;

bool isPaused = false;

// Physics Variables
float eggSpeed = 1.6f;      // Falling Speed Step
float originalSpeed = 1.6f; // Default Speed Backup
float airflow = 0.0f;       // Horizontal Wind Drift
int perkEffectTimer = 0;    // Powerup Duration Counter

// Chicken Movement
float chX[4] = {-60, 20, -30, 50};
float chDir[4] = {0.65f, -0.45f, 0.85f, -0.55f}; // Individual Walk Speeds

// Egg Properties Group
struct Egg {
    float x, y;
    int type; // Type Index
    bool active, broken;
    int brokenTimer;
} gameEgg;

// ==========================================
// 2. SOUND & FILE SYSTEM
// ==========================================
void playBGM() {
    // Loop Background Music
    mciSendString("close all", NULL, 0, NULL);
    mciSendString("open \"bg_music.wav\" type waveaudio alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
}

void playSFX(std::string file) {
    // Play Overlapping Sound Effects
    PlaySound(TEXT(file.c_str()), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}

void saveScore() {
    // Save High Score File
    std::ofstream f("highscore.txt");
    if (f.is_open()) f << highScore;
    f.close();
}

void loadScore() {
    // Load High Score File
    std::ifstream f("highscore.txt");
    if (f.is_open()) f >> highScore;
    f.close();
}

// ==========================================
// 3. GRAPHICS RENDERING
// ==========================================
void drawText(float x, float y, std::string str, void* font = GLUT_BITMAP_HELVETICA_18) {
    // Render Screen Text
    glRasterPos2f(x, y);
    for (char c : str) glutBitmapCharacter(font, c);
}

void drawCircle(float x, float y, float rx, float ry) {
    // Trigonometric Poly Circle
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 10) {
        float rad = i * 0.01745f; // Radian Conversion Formula
        glVertex2f(x + cos(rad) * rx, y + sin(rad) * ry); // Point Math Formula
    }
    glEnd();
}

void drawHeart(float x, float y, float r) {
    // Mathematical Heart Formula
    glBegin(GL_POLYGON);
    for (int i = 0; i < 360; i += 5) {
        float rad = i * 0.01745f;
        float hx = 16 * sin(rad) * sin(rad) * sin(rad);
        float hy = 13 * cos(rad) - 5 * cos(2 * rad) - 2 * cos(3 * rad) - cos(4 * rad);

        glVertex2f(x + (hx * r * 0.18f), y + (hy * r * 0.18f));
    }
    glEnd();
}

void drawChicken(float x, float y) {
    // Render Chicken Hierarchy
    glColor3f(1, 1, 1); drawCircle(x, y, 10, 7); // Body
    drawCircle(x + 8, y + 5, 5, 5);              // Head
    glColor3f(0, 0, 0); drawCircle(x + 10, y + 6, 0.8, 0.8); // Eye

    // Draw Beak Triangle
    glColor3f(1.0, 0.5, 0.0);
    glBegin(GL_TRIANGLES);
    glVertex2f(x+13, y+5); glVertex2f(x+17, y+6); glVertex2f(x+17, y+4);
    glEnd();

    glColor3f(0.8, 0, 0);
    drawCircle(x + 8, y + 10, 2, 3); // Crown
}

void drawRealisticEgg(float x, float y, int type) {
    if (type == 3) { // Draw Poop Swirls
        glColor3f(0.4, 0.2, 0.1);
        drawCircle(x, y - 2, 5, 3);
        drawCircle(x, y, 4, 2.5);
        drawCircle(x, y + 2, 2.5, 2);
    } else {
        // Color Type Mapping
        if (type == 0) glColor3f(1, 1, 1);
        else if (type == 1) glColor3f(0.2, 0.5, 1);
        else if (type == 2) glColor3f(1, 0.8, 0);
        else if (type == 4) glColor3f(0, 1, 1);
        else if (type == 5) glColor3f(1, 0, 1);
        else if (type == 6) glColor3f(0, 1, 0);
        else if (type == 7) glColor3f(0.8, 0, 0);

        // Tapered Egg Shell Shape
        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++) {
            float rad = i * 0.01745f;
            float rx = 3.2f, ry = 4.8f;
            if (i > 0 && i < 180) ry = 5.2f; // Top Radius Taper
            glVertex2f(x + cos(rad) * rx, y + sin(rad) * ry);
        }
        glEnd();

        // Alpha Blending Reflection
        glColor4f(1, 1, 1, 0.5);
        drawCircle(x - 1.2, y + 2, 0.7, 1.2);
    }
}

void drawBasket(float x, float w) {
    // Render Catcher Geometry
    glColor3f(0.5, 0.3, 0.1);
    glRectf(x - w, -85, x + w, -68); // Solid Base Rectangle

    glColor3f(0.4, 0.2, 0.0);
    glRectf(x - w - 2, -70, x + w + 2, -65); // Rim Line

    // Procedural Line Grid
    glColor3f(0.3, 0.1, 0.0);
    glBegin(GL_LINES);
    for (float i = x - w + 2; i < x + w; i += 5) { glVertex2f(i, -85); glVertex2f(i, -70); }
    glEnd();
}

// ==========================================
// 4. GAME ENGINE LOGIC
// ==========================================
void resetEgg() {
    // Random Drop Placement
    int source = rand() % 4;
    gameEgg.x = chX[source];
    gameEgg.y = (source < 2) ? 78 : 48;

    // Random Loot Generation Weights
    int r = rand() % 100;
    if (r < 45) gameEgg.type = 0;
    else if (r < 55) gameEgg.type = 1;
    else if (r < 62) gameEgg.type = 2;
    else if (r < 75) gameEgg.type = 3;
    else if (r < 82) gameEgg.type = 4;
    else if (r < 88) gameEgg.type = 5;
    else if (r < 94) gameEgg.type = 6;
    else gameEgg.type = 7;

    gameEgg.active = true; gameEgg.broken = false; gameEgg.brokenTimer = 0;

    // Wind Force Value
    airflow = ((rand() % 21) - 10) / 30.0f;
}

void update(int v) {
    if (currentState == PLAYING && !isPaused) {
        // Move Chicken Patrols
        for (int i = 0; i < 4; i++) {
            chX[i] += chDir[i];
            if (chX[i] > 85 || chX[i] < -85) chDir[i] *= -1; // Screen Bounce Check
        }

        if (!gameEgg.broken) {
            gameEgg.y -= eggSpeed; // Falling Displacement Math
            gameEgg.x += airflow;

            // Bounding Box Collision Check
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

                eggSpeed += 0.02; originalSpeed += 0.01; // Core Velocity Scaling
                resetEgg();
            }

            // Ground Boundary Check
            if (gameEgg.y < -85) {
                gameEgg.broken = true;
                playSFX("break.wav");
                if (gameEgg.type <= 2) lives--; // Health Deduction Logic
            }
        } else {
            gameEgg.brokenTimer++;
            if (gameEgg.brokenTimer > 15) resetEgg(); // Splat Splashes Delay
        }

        if (perkEffectTimer > 0) {
            perkEffectTimer--;
            if (perkEffectTimer == 0) { basketWidth = 22; eggSpeed = originalSpeed; } // Buff Expiry Reset
        }

        if (lives <= 0 || remainingTime <= 0) {
            currentState = GAMEOVER;
            if (score > highScore) { highScore = score; saveScore(); }
        }
    }
    glutPostRedisplay(); // Request Frame Redraw
    glutTimerFunc(16, update, 0); // 60 FPS Callback Timer
}

void clockTick(int v) {
    if (currentState == PLAYING && !isPaused) remainingTime--;
    glutTimerFunc(1000, clockTick, 0); // 1-Second Step Countdown
}

// ==========================================
// 5. CALLBACK DISPLAY INTERFACE ENGINE
// ==========================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT); // Wipe Previous Frame Pixels
    if (currentState == MENU) {

        // Canvas Background Buffer Fill
        glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Menu Interface Text Strings
        glColor3f(1.0f, 0.85f, 0.2f);
        drawText(-46, 45, "CATCH THE EGGS - NEW EDITION", GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(0.3f, 0.4f, 0.5f);
        drawText(-50, 35, "================================================");

        glColor3f(0.9f, 0.9f, 0.9f);
        drawText(-26, 12, "[ S ]   START GAME");
        drawText(-26, -3, "[ H ]   HOW TO PLAY");
        drawText(-26, -18, "[ Q ]   QUIT DESKTOP");

        glColor3f(0.4f, 0.7f, 1.0f);
        drawText(-30, -65, "Created by : Momitu & Labony");

        if(highScore > 0) {
            std::stringstream hs; hs << "★ BEST SCORE : " << highScore << " ★";
            glColor3f(1.0f, 0.6f, 0.0f);
            drawText(-23, -42, hs.str());
        }
    }
    else if (currentState == HELP) {
        glClearColor(0.1, 0.1, 0.1, 1);
        glColor3f(1, 1, 1);
        drawText(-30, 60, "--- HOW TO PLAY ---", GLUT_BITMAP_TIMES_ROMAN_24);
        drawText(-85, 30, "-> Catch White/Blue/Gold eggs for points.");
        drawText(-85, 15, "-> AVOID THE POOP (Brown). Lose 10 points!");
        drawText(-85, 0, "-> Missing any good egg costs 1 LIFE.");
        drawText(-85, -15, "-> Blue/Pink/Green eggs are special PERKS.");
        drawText(-20, -60, "Press 'B' to Go Back");
    } else {
        // Gameplay Stage Background Colors
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Grass Bounds Shape Rectangle
        glColor3f(0.2f, 0.8f, 0.2f);
        glRectf(-100, -100, 100, -85);
        if(lives == 1) glClearColor(0.7, 0.3, 0.3, 1); // Dynamic Warning Backdrop Color
        else glClearColor(0.5, 0.8, 1.0, 1);

        // Bamboo Platform Shapes
        glColor3f(0.3, 0.2, 0.1);
        glRectf(-100, 78, 100, 81); glRectf(-100, 48, 100, 51);

        for (int i = 0; i < 4; i++) drawChicken(chX[i], (i < 2 ? 86 : 56));

        // Display Score Stream Formatting
        glColor3f(0, 0, 0);
        std::stringstream ss, st;
        ss << "Score: " << score << "  Best: " << highScore; drawText(-95, 92, ss.str());
        st << "Time: " << remainingTime << "s"; drawText(70, 92, st.str());

        // Low Health Text Condition Flag
        if (lives == 1) {
            glColor3f(1.0f, 0.0f, 0.0f);
        } else {
            glColor3f(0.0f, 0.0f, 0.0f);
        }
        drawText(15, 92, "HEALTH:");

        // Serial Heart Rendering Array Loop
        for(int i = 0; i < lives; i++) {
            glColor3f(0.9f, 0.1f, 0.1f);
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
    glutSwapBuffers(); // Double Buffer Flipping
}

void keyboard(unsigned char k, int x, int y) {
    // Standard ASCII Inputs Interface
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
    if (k == 'q' || k == 'Q' || k == 27) exit(0); // 27 = Escape Key Index
}

void special(int k, int x, int y) {
    // Non-ASCII Function Key Mapping Inputs
    if (k == GLUT_KEY_LEFT && basketX > -80) basketX -= 10;
    if (k == GLUT_KEY_RIGHT && basketX < 80) basketX += 10;
}

void mouse(int x, int y) {
    // World Space View Scale Normalization Matrix
    basketX = (x / (float)glutGet(GLUT_WINDOW_WIDTH)) * 200.0f - 100.0f;
    if (basketX > 80) basketX = 80; if (basketX < -80) basketX = -80; // Hard Clamping Edge Bounds
}

int main(int argc, char** argv) {
    srand(time(NULL)); // System Clock Entropy Seed Setup
    loadScore();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); // Double Buffer Window Target Specification
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Catch The Eggs - HD Edition");

    // Grid Space Boundaries Mapping (Left, Right, Bottom, Top)
    gluOrtho2D(-100, 100, -100, 100);

    // Core Engine Callbacks Pipeline Linkages
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutPassiveMotionFunc(mouse); // Track Mouse Coordinates Without Click Flags
    glutTimerFunc(16, update, 0);
    glutTimerFunc(1000, clockTick, 0);

    glutMainLoop(); // System Framework Thread Loop
    return 0;
}
