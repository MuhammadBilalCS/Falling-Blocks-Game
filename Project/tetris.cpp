#include "raylib.h"
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>

const int COLS = 10;
const int ROWS = 20;
const int CELL = 32;
const int SIDEBARWIDTH = 200;
const int TOTALWIDTH = COLS * CELL + SIDEBARWIDTH;
const int TOTALHEIGHT = ROWS * CELL;
const float FALLTIME = 0.5f;
const int MAX_UNDO = 5;
const int MAX_LEVEL = 20;
const int MAX_NAME_LENGTH = 12;
const float EXCEPTION_TIMEOUT = 3.0f;
const int MAX_LEADERBOARD = 5;


static const float MUSIC_TICK = 0.125f; 

static const float MELODY[] = {
   
    659.25f, 0, 493.88f, 523.25f, 587.33f, 0, 523.25f, 493.88f,
    440.00f, 0, 440.00f, 523.25f, 659.25f, 0, 587.33f, 523.25f,
    493.88f, 0, 493.88f, 523.25f, 587.33f, 0, 659.25f, 0,
    523.25f, 0, 440.00f, 0, 440.00f, 0, 0,        0,
    
    587.33f, 0, 698.46f, 0, 880.00f, 0, 783.99f, 698.46f,
    659.25f, 0, 659.25f, 523.25f, 659.25f, 0, 587.33f, 523.25f,
    493.88f, 0, 493.88f, 523.25f, 587.33f, 0, 659.25f, 0,
    523.25f, 0, 440.00f, 0, 440.00f, 0, 0,        0,
   
    659.25f, 0, 493.88f, 523.25f, 587.33f, 0, 523.25f, 493.88f,
    440.00f, 0, 440.00f, 523.25f, 659.25f, 0, 587.33f, 523.25f,
    493.88f, 0, 493.88f, 523.25f, 587.33f, 0, 659.25f, 0,
    523.25f, 0, 440.00f, 0, 440.00f, 0, 0,        0,
};
static const int MELODY_LEN = (int)(sizeof(MELODY) / sizeof(MELODY[0]));


static const float BASS[] = {
    220.00f, 0, 0, 0, 185.00f, 0, 0, 0,
    174.61f, 0, 0, 0, 196.00f, 0, 0, 0,
    220.00f, 0, 0, 0, 185.00f, 0, 0, 0,
    174.61f, 0, 0, 0, 196.00f, 0, 0, 0,
};
static const int BASS_LEN = (int)(sizeof(BASS) / sizeof(BASS[0]));

static Wave generateTone(float freq, float duration, float volume, int sampleRate = 44100)
{
    int sampleCount = (int)(duration * sampleRate);
    if (sampleCount <= 0) sampleCount = 1;

    Wave wave;
    wave.frameCount = sampleCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = (short*)RL_MALLOC(sampleCount * sizeof(short));

    short* data = (short*)wave.data;

    if (freq <= 0.f) {
        // silence
        for (int i = 0; i < sampleCount; i++) data[i] = 0;
    } else {
        float period = (float)sampleRate / freq;
        // Envelope: short attack + decay to avoid clicking
        int attack = (int)(sampleRate * 0.005f);
        int release = (int)(sampleRate * 0.03f);
        for (int i = 0; i < sampleCount; i++) {
            float env = 1.0f;
            if (i < attack) env = (float)i / attack;
            else if (i > sampleCount - release) env = (float)(sampleCount - i) / release;

            float t = fmodf((float)i, period) / period;
            float sample = (t < 0.5f) ? 1.0f : -1.0f; // square wave
            data[i] = (short)(sample * env * volume * 32000.0f);
        }
    }
    return wave;
}

static Sound makeSFX(float freq, float duration, float volume = 0.4f) {
    Wave w = generateTone(freq, duration, volume);
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}


class ChiptunePlayer {
private:
    AudioStream stream;
    int sampleRate;
    int melodyIdx;
    int bassIdx;
    float melodyTimer;
    float bassTimer;
    float melodyFreq;
    float bassFreq;
    float melodyPhase;
    float bassPhase;
    bool playing;

    static const int BUFFER_SIZE = 4096;
    short buffer[4096];

public:
    ChiptunePlayer() : sampleRate(44100), melodyIdx(0), bassIdx(0),
        melodyTimer(0), bassTimer(0), melodyFreq(0), bassFreq(0),
        melodyPhase(0), bassPhase(0), playing(false) {}

    void init() {
        InitAudioDevice();
        stream = LoadAudioStream(sampleRate, 16, 1);
        SetAudioStreamVolume(stream, 0.35f);
        PlayAudioStream(stream);
        playing = true;
        melodyFreq = MELODY[0];
        bassFreq = BASS[0];
    }

    void update(float dt) {
        if (!playing) return;
        if (!IsAudioStreamProcessed(stream)) return;

        
        float melodyPeriod = (melodyFreq > 0) ? (float)sampleRate / melodyFreq : 0;
        float bassPeriod   = (bassFreq   > 0) ? (float)sampleRate / bassFreq   : 0;

        for (int i = 0; i < BUFFER_SIZE; i++) {
            float t = (float)i / sampleRate;

            
            float mSample = 0.f;
            if (melodyFreq > 0 && melodyPeriod > 0) {
                melodyPhase += 1.0f / melodyPeriod;
                if (melodyPhase >= 1.f) melodyPhase -= 1.f;
                mSample = (melodyPhase < 0.5f) ? 0.6f : -0.6f;
            }

            
            float bSample = 0.f;
            if (bassFreq > 0 && bassPeriod > 0) {
                bassPhase += 1.0f / bassPeriod;
                if (bassPhase >= 1.f) bassPhase -= 1.f;
                bSample = (bassPhase < 0.5f)
                    ? (4.f * bassPhase - 1.f) * 0.3f
                    : (3.f - 4.f * bassPhase) * 0.3f;
            }

            buffer[i] = (short)((mSample + bSample) * 32000.f * 0.5f);

            
            melodyTimer += 1.0f / sampleRate;
            if (melodyTimer >= MUSIC_TICK) {
                melodyTimer -= MUSIC_TICK;
                melodyIdx = (melodyIdx + 1) % MELODY_LEN;
                melodyFreq = MELODY[melodyIdx];
                if (melodyFreq > 0) melodyPeriod = (float)sampleRate / melodyFreq;
            }

            
            bassTimer += 1.0f / sampleRate;
            if (bassTimer >= MUSIC_TICK * 2.f) {
                bassTimer -= MUSIC_TICK * 2.f;
                bassIdx = (bassIdx + 1) % BASS_LEN;
                bassFreq = BASS[bassIdx];
                if (bassFreq > 0) bassPeriod = (float)sampleRate / bassFreq;
            }
        }

        UpdateAudioStream(stream, buffer, BUFFER_SIZE);
    }

    void stop() {
        if (playing) {
            StopAudioStream(stream);
            UnloadAudioStream(stream);
            CloseAudioDevice();
            playing = false;
        }
    }

    ~ChiptunePlayer() {}
};



class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& msg) : std::runtime_error(msg) {}
};

template<typename T>
class GameStack {
private:
    std::vector<T> data;
public:
    void push(const T& item) {
        if ((int)data.size() >= MAX_UNDO) data.erase(data.begin());
        data.push_back(item);
    }
    void pop() { if (!data.empty()) data.pop_back(); }
    T& top() {
        if (data.empty()) throw GameException("Undo stack is empty!");
        return data.back();
    }
    bool empty() const { return data.empty(); }
    int size() const { return (int)data.size(); }
    void clear() { data.clear(); }
};

struct ScoreEntry {
    std::string name;
    int score;
    int level;
    bool operator>(const ScoreEntry& o) const { return score > o.score; }
    bool operator==(const ScoreEntry& o) const { return name == o.name && score == o.score; }
    bool operator!=(const ScoreEntry& o) const { return !(*this == o); }
};

class Leaderboard {
private:
    std::vector<ScoreEntry> entries;
    void addScore(const std::string& name, int score, int level) {
        entries.push_back({name, score, level});
        std::sort(entries.begin(), entries.end(),
            [](const ScoreEntry& a, const ScoreEntry& b){ return a > b; });
        if ((int)entries.size() > MAX_LEADERBOARD) entries.resize(MAX_LEADERBOARD);
    }
public:
    void addOrUpdateScore(const std::string& name, int score, int level) {
        if (name.empty()) return;
        for (auto& entry : entries) {
            if (entry.name == name) {
                if (score > entry.score) {
                    entry.score = score; entry.level = level;
                    std::sort(entries.begin(), entries.end(),
                        [](const ScoreEntry& a, const ScoreEntry& b){ return a > b; });
                }
                return;
            }
        }
        addScore(name, score, level);
    }
    const std::vector<ScoreEntry>& getEntries() const { return entries; }
    friend class Renderer;
};

class IDrawable { public: virtual void draw() const = 0; virtual ~IDrawable() {} };
class IUpdatable { public: virtual void update(float dt) = 0; virtual ~IUpdatable() {} };

static const Color PIECE_COLORS[8] = {
    {15, 15, 20, 255},
    {0, 240, 240, 255},
    {0, 0, 240, 255},
    {240, 160, 0, 255},
    {240, 240, 0, 255},
    {0, 240, 0, 255},
    {160, 0, 240, 255},
    {240, 0, 0, 255}
};

static const int SHAPES[7][4][4][2] = {
    {{{0,0},{0,1},{0,2},{0,3}},{{0,2},{1,2},{2,2},{3,2}},{{2,0},{2,1},{2,2},{2,3}},{{0,1},{1,1},{2,1},{3,1}}},
    {{{0,0},{1,0},{1,1},{1,2}},{{0,1},{0,2},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,0},{2,1}}},
    {{{0,2},{1,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{1,2},{2,0}},{{0,0},{0,1},{1,1},{2,1}}},
    {{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}},{{0,0},{0,1},{1,0},{1,1}}},
    {{{0,1},{0,2},{1,0},{1,1}},{{0,1},{1,1},{1,2},{2,2}},{{1,1},{1,2},{2,0},{2,1}},{{0,0},{1,0},{1,1},{2,1}}},
    {{{0,1},{1,0},{1,1},{1,2}},{{0,1},{1,1},{1,2},{2,1}},{{1,0},{1,1},{1,2},{2,1}},{{0,1},{1,0},{1,1},{2,1}}},
    {{{0,0},{0,1},{1,1},{1,2}},{{0,2},{1,1},{1,2},{2,1}},{{1,0},{1,1},{2,1},{2,2}},{{0,1},{1,0},{1,1},{2,0}}}
};

class Board : public IDrawable {
private:
    int grid[ROWS][COLS];
public:
    Board() { clear(); }
    Board(const Board& other) { std::memcpy(grid, other.grid, sizeof(grid)); }
    Board& operator=(const Board& other) {
        if (this != &other) std::memcpy(grid, other.grid, sizeof(grid));
        return *this;
    }
    bool operator==(const Board& other) const { return std::memcmp(grid, other.grid, sizeof(grid)) == 0; }
    bool operator!=(const Board& other) const { return !(*this == other); }

    void clear() { std::memset(grid, 0, sizeof(grid)); }
    int getCell(int r, int c) const {
        if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return -1;
        return grid[r][c];
    }
    void setCell(int r, int c, int val) {
        if (r >= 0 && r < ROWS && c >= 0 && c < COLS) grid[r][c] = val;
    }
    bool isValidPos(int r, int c) const {
        return r >= 0 && r < ROWS && c >= 0 && c < COLS && grid[r][c] == 0;
    }
    int clearLines() {
        int cleared = 0;
        for (int r = ROWS - 1; r >= 0;) {
            bool full = true;
            for (int c = 0; c < COLS; c++) if (grid[r][c] == 0) { full = false; break; }
            if (full) {
                for (int rr = r; rr > 0; rr--)
                    for (int c = 0; c < COLS; c++) grid[rr][c] = grid[rr-1][c];
                for (int c = 0; c < COLS; c++) grid[0][c] = 0;
                cleared++;
            } else r--;
        }
        return cleared;
    }
    void draw() const override {
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) {
                int ci = grid[r][c];
                Color col = (ci == 0) ? Color{30, 30, 40, 255} : PIECE_COLORS[ci];
                DrawRectangle(c*CELL, r*CELL, CELL-1, CELL-1, col);
            }
        for (int r = 0; r <= ROWS; r++)
            DrawLine(0, r*CELL, COLS*CELL, r*CELL, Color{50, 50, 60, 255});
        for (int c = 0; c <= COLS; c++)
            DrawLine(c*CELL, 0, c*CELL, ROWS*CELL, Color{50, 50, 60, 255});
    }
};

class BasePiece : public IDrawable {
protected:
    int type, rotation, row, col;
public:
    BasePiece(int t) : type(t), rotation(0), row(0), col(3) {}
    BasePiece(const BasePiece& o) : type(o.type), rotation(o.rotation), row(o.row), col(o.col) {}
    virtual ~BasePiece() {}

    int getType() const { return type; }
    int getRotation() const { return rotation; }
    int getRow() const { return row; }
    int getCol() const { return col; }
    void setRow(int r) { row = r; }
    void setCol(int c) { col = c; }
    void setRotation(int r) { rotation = (r + 4) % 4; }

    void getCells(int cells[4][2]) const {
        for (int i = 0; i < 4; i++) {
            cells[i][0] = row + SHAPES[type][rotation][i][0];
            cells[i][1] = col + SHAPES[type][rotation][i][1];
        }
    }
    bool fits(const Board& board, int dr, int dc, int drot) const {
        int newRot = (rotation + drot + 4) % 4;
        int newRow = row + dr, newCol = col + dc;
        for (int i = 0; i < 4; i++) {
            int r = newRow + SHAPES[type][newRot][i][0];
            int c = newCol + SHAPES[type][newRot][i][1];
            if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return false;
            if (board.getCell(r, c) != 0) return false;
        }
        return true;
    }
    void lock(Board& board) const {
        int cells[4][2]; getCells(cells);
        for (int i = 0; i < 4; i++) board.setCell(cells[i][0], cells[i][1], type + 1);
    }
    int ghostDropRow(const Board& board) const {
        int dr = 0;
        while (fits(board, dr+1, 0, 0)) dr++;
        return row + dr;
    }
    bool operator==(const BasePiece& o) const {
        return type == o.type && rotation == o.rotation && row == o.row && col == o.col;
    }
    bool operator!=(const BasePiece& o) const { return !(*this == o); }

    virtual void draw() const override = 0;
    virtual void drawPreview(int px, int py) const = 0;
};

class Piece : public BasePiece {
public:
    Piece() : BasePiece(0) {}
    Piece(int t) : BasePiece(t) {}
    Piece(const Piece& o) : BasePiece(o) {}
    Piece& operator=(const Piece& o) {
        if (this != &o) { type = o.type; rotation = o.rotation; row = o.row; col = o.col; }
        return *this;
    }
    void draw() const override {
        int cells[4][2]; getCells(cells);
        Color c = PIECE_COLORS[type+1];
        for (int i = 0; i < 4; i++)
            DrawRectangle(cells[i][1]*CELL, cells[i][0]*CELL, CELL-1, CELL-1, c);
    }
    void drawWithGhost(const Board& board) const {
        int ghost = ghostDropRow(board);
        int cells[4][2]; getCells(cells);
        Color c = PIECE_COLORS[type+1];
        int diff = ghost - row;
        for (int i = 0; i < 4; i++) {
            int gr = cells[i][0] + diff, gc = cells[i][1];
            DrawRectangle(gc*CELL, gr*CELL, CELL-1, CELL-1, Color{c.r, c.g, c.b, 55});
        }
        for (int i = 0; i < 4; i++)
            DrawRectangle(cells[i][1]*CELL, cells[i][0]*CELL, CELL-1, CELL-1, c);
    }
    void drawPreview(int px, int py) const override {
        Color c = PIECE_COLORS[type+1];
        for (int i = 0; i < 4; i++) {
            int r = SHAPES[type][rotation][i][0];
            int cc = SHAPES[type][rotation][i][1];
            DrawRectangle(px + cc*24, py + r*24, 23, 23, c);
        }
    }
};

struct GameSnapshot {
    Board board; Piece current;
    int score, linesTotal, level;
    float fallSpeed;
};

class Renderer {
public:
    Renderer() = delete;
    static void drawLeaderboard(const Leaderboard& lb, int x, int y) {
        DrawText("TOP SCORES", x, y, 16, LIGHTGRAY);
        y += 22;
        const auto& entries = lb.entries;
        if (entries.empty()) { DrawText("No scores yet!", x, y, 12, Color{100, 100, 120, 255}); return; }
        for (int i = 0; i < (int)entries.size(); i++) {
            std::string line = std::to_string(i+1) + ". " + entries[i].name + " " + std::to_string(entries[i].score);
            Color c = (i==0) ? Color{255,215,0,255} : (i==1) ? Color{192,192,192,255} : (i==2) ? Color{205,127,50,255} : YELLOW;
            DrawText(line.c_str(), x, y + i*18, 13, c);
        }
    }
    static void drawExceptionBox(const std::string& msg, float timer) {
        if (timer <= 0.f || msg.empty()) return;
        DrawRectangle(10, TOTALHEIGHT-65, COLS*CELL-20, 55, Color{180,0,0,210});
        DrawRectangleLines(10, TOTALHEIGHT-65, COLS*CELL-20, 55, Color{255,80,80,255});
        DrawText("! EXCEPTION:", 20, TOTALHEIGHT-60, 14, Color{255,200,200,255});
        DrawText(msg.c_str(), 20, TOTALHEIGHT-42, 13, WHITE);
        float ratio = timer / EXCEPTION_TIMEOUT;
        DrawRectangle(10, TOTALHEIGHT-15, (int)((COLS*CELL-20)*ratio), 6, Color{255,80,80,180});
    }
    static void drawExceptionBlockScreen(const std::string& msg) {
        if (msg.empty()) return;
        DrawRectangle(0, 0, TOTALWIDTH, TOTALHEIGHT, Color{20,0,0,230});
        DrawRectangleLines(20, TOTALHEIGHT/2-80, TOTALWIDTH-40, 160, Color{255,80,80,255});
        DrawRectangle(20, TOTALHEIGHT/2-80, TOTALWIDTH-40, 160, Color{80,0,0,220});
        DrawText("EXCEPTION OCCURRED", TOTALWIDTH/2-130, TOTALHEIGHT/2-65, 20, Color{255,100,100,255});
        DrawLine(30, TOTALHEIGHT/2-40, TOTALWIDTH-30, TOTALHEIGHT/2-40, Color{255,80,80,120});
        DrawText(msg.c_str(), TOTALWIDTH/2-140, TOTALHEIGHT/2-28, 14, WHITE);
        DrawText("Press ENTER to dismiss and continue", TOTALWIDTH/2-145, TOTALHEIGHT/2+20, 14, Color{200,200,200,255});
    }
};

struct SFXBundle {
    Sound lineClear;
    Sound tetris;       // 4 lines
    Sound hardDrop;
    Sound gameOver;
    Sound undo;

    void init() {
        // Line clear: ascending arpeggio
        lineClear = makeSFX(880.0f, 0.12f, 0.45f);
        // Tetris (4 lines): triumphant chord sweep
        tetris    = makeSFX(1046.5f, 0.25f, 0.5f);
        // Hard drop: low thud
        hardDrop  = makeSFX(150.0f, 0.06f, 0.35f);
        // Game over: descending note
        gameOver  = makeSFX(220.0f, 0.4f, 0.5f);
        // Undo: click
        undo      = makeSFX(440.0f, 0.05f, 0.3f);
    }

    void unload() {
        UnloadSound(lineClear);
        UnloadSound(tetris);
        UnloadSound(hardDrop);
        UnloadSound(gameOver);
        UnloadSound(undo);
    }
};

class Game : public IUpdatable, public IDrawable {
private:
    Board board;
    Piece current, next;
    Leaderboard leaderboard;
    GameStack<GameSnapshot> undoStack;
    SFXBundle* sfx;   // pointer — injected from main

    bool gameOver;
    int score, level, linesTotal;
    float fallTimer, fallSpeed;

    bool askingName;
    std::string playerName;
    float cursorTimer;
    bool showCursor;

    std::string exceptionMsg;
    float exceptionTimer;
    bool exceptionBlocking;

    std::map<std::string, std::function<void()>> eventCallbacks;

    void triggerEvent(const std::string& name) {
        auto it = eventCallbacks.find(name);
        if (it != eventCallbacks.end()) it->second();
    }

    void setupCallbacks() {
        eventCallbacks["lineClear"] = [this]() {};
        eventCallbacks["gameOver"]  = [this]() {};
        eventCallbacks["undo"]      = [this]() {};
    }

    void saveSnapshot() {
        GameSnapshot snap;
        snap.board = board; snap.current = current;
        snap.score = score; snap.linesTotal = linesTotal;
        snap.level = level; snap.fallSpeed = fallSpeed;
        undoStack.push(snap);
    }

    void addScore(int lines) {
        if (lines <= 0) return;
        if (lines > 4) throw GameException("clearLines() ne 4 se zyada lines di heein to — board corrupt!");
        static const int pts[5] = {0, 100, 300, 500, 800};
        score += pts[lines] * level;
        if (score < 0) score = 0;
        linesTotal += lines;
        level = linesTotal / 10 + 1;
        if (level > MAX_LEVEL) level = MAX_LEVEL;
        fallSpeed = FALLTIME / (1.f + (level-1)*0.15f);
        if (sfx) {
            if (lines >= 4) PlaySound(sfx->tetris);
            else            PlaySound(sfx->lineClear);
        }
        triggerEvent("lineClear");
    }

    void spawnNext() {
        current = next;
        next = Piece(rand() % 7);
        if (!current.fits(board, 0, 0, 0)) {
            gameOver = true;
            std::string nameToSave = playerName.empty() ? "UNKNOWN" : playerName;
            leaderboard.addOrUpdateScore(nameToSave, score, level);
            if (sfx) PlaySound(sfx->gameOver);
            triggerEvent("gameOver");
        }
    }

    void handleException(const std::string& msg) {
        exceptionMsg = msg;
        exceptionTimer = EXCEPTION_TIMEOUT;
        exceptionBlocking = true;
        std::cout << "[Exception] " << msg << std::endl;
    }

public:
    Game(SFXBundle* sfxBundle = nullptr) : sfx(sfxBundle) {
        exceptionMsg = ""; exceptionTimer = 0.f; exceptionBlocking = false;
        setupCallbacks();
        reset();
    }
    ~Game() {}

    void setSFX(SFXBundle* s) { sfx = s; }

    void reset() {
        board.clear();
        current = Piece(rand() % 7);
        next    = Piece(rand() % 7);
        gameOver = false;
        score = 0; level = 1; linesTotal = 0;
        fallTimer = 0.f; fallSpeed = FALLTIME;
        undoStack.clear();
        askingName = true;
        playerName = ""; cursorTimer = 0.f; showCursor = true;
        exceptionMsg = ""; exceptionTimer = 0.f; exceptionBlocking = false;
    }

    void setLevel(int lvl) {
        try {
            if (lvl < 1 || lvl > MAX_LEVEL)
                throw GameException("Level 1 se " + std::to_string(MAX_LEVEL) + " ke beech tak hona chahiye!");
            level = lvl;
            fallSpeed = FALLTIME / (1.f + (level-1)*0.15f);
        } catch (const GameException& e) { handleException(e.what()); }
    }

    void undo() {
        if (!undoStack.empty()) {
            try {
                GameSnapshot& snap = undoStack.top();
                board = snap.board; current = snap.current;
                score = snap.score; linesTotal = snap.linesTotal;
                level = snap.level; fallSpeed = snap.fallSpeed;
                undoStack.pop();
                gameOver = false;
                if (sfx) PlaySound(sfx->undo);
                triggerEvent("undo");
            } catch (const GameException& e) { handleException(e.what()); }
        }
    }

    void update(float dt) override {
        if (dt <= 0.f) return;
        if (exceptionBlocking) return;
        if (askingName) {
            cursorTimer += dt;
            if (cursorTimer >= 0.5f) { cursorTimer = 0.f; showCursor = !showCursor; }
            if (exceptionTimer > 0.f) exceptionTimer -= dt;
            return;
        }
        if (exceptionTimer > 0.f) exceptionTimer -= dt;
        if (gameOver) return;

        fallTimer += dt;
        if (fallTimer >= fallSpeed) {
            fallTimer = 0.f;
            if (current.fits(board, 1, 0, 0))
                current.setRow(current.getRow() + 1);
            else {
                saveSnapshot();
                current.lock(board);
                int cleared = board.clearLines();
                if (cleared) {
                    try { addScore(cleared); }
                    catch (const GameException& e) { handleException(e.what()); }
                }
                spawnNext();
            }
        }
    }

    void handleInput() {
        if (exceptionBlocking) {
            if (IsKeyPressed(KEY_ENTER)) exceptionBlocking = false;
            return;
        }
        if (askingName) {
            int ch = GetCharPressed();
            while (ch > 0) {
                if (ch >= 32 && ch <= 126 && (int)playerName.size() < MAX_NAME_LENGTH)
                    playerName += (char)ch;
                ch = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && !playerName.empty()) playerName.pop_back();
            if (IsKeyPressed(KEY_ENTER) && !playerName.empty()) askingName = false;
            return;
        }
        if (gameOver) {
            if (IsKeyPressed(KEY_R)) reset();
            if (IsKeyPressed(KEY_U)) undo();
            return;
        }

        if (IsKeyPressed(KEY_LEFT)  && current.fits(board, 0, -1, 0)) current.setCol(current.getCol()-1);
        if (IsKeyPressed(KEY_RIGHT) && current.fits(board, 0,  1, 0)) current.setCol(current.getCol()+1);
        if (IsKeyPressed(KEY_DOWN)  && current.fits(board, 1,  0, 0)) {
            current.setRow(current.getRow()+1);
            score++; if (score < 0) score = 0;
        }
        if (IsKeyPressed(KEY_UP) && current.fits(board, 0, 0, 1))
            current.setRotation(current.getRotation()+1);

        if (IsKeyPressed(KEY_SPACE)) {
            saveSnapshot();
            while (current.fits(board, 1, 0, 0)) {
                current.setRow(current.getRow()+1);
                score += 2;
            }
            if (score < 0) score = 0;
            if (sfx) PlaySound(sfx->hardDrop);
            current.lock(board);
            int cleared = board.clearLines();
            if (cleared) {
                try { addScore(cleared); }
                catch (const GameException& e) { handleException(e.what()); }
            }
            spawnNext();
        }

        if (IsKeyPressed(KEY_U)) undo();

        // M = mute toggle (handled in main, just a hint here)
    }

    void draw() const override {
        if (exceptionBlocking) {
            board.draw();
            Renderer::drawExceptionBlockScreen(exceptionMsg);
            return;
        }
        if (askingName) {
            DrawRectangle(0, 0, TOTALWIDTH, TOTALHEIGHT, Color{10, 10, 20, 255});
            DrawText("TETRIS", TOTALWIDTH/2-75, 80, 50, Color{0, 220, 220, 255});
            DrawText("Enter your name:", TOTALWIDTH/2-115, 180, 22, LIGHTGRAY);
            DrawRectangle(TOTALWIDTH/2-160, 215, 320, 48, Color{30, 30, 50, 255});
            DrawRectangleLines(TOTALWIDTH/2-160, 215, 320, 48, Color{0, 180, 220, 255});
            std::string display = playerName + (showCursor ? "|" : " ");
            DrawText(display.c_str(), TOTALWIDTH/2-145, 228, 24, Color{255, 220, 0, 255});
            if (playerName.empty())
                DrawText("Type name then press ENTER", TOTALWIDTH/2-135, 278, 16, Color{150, 150, 170, 255});
            else
                DrawText("Press ENTER to start!", TOTALWIDTH/2-110, 278, 18, Color{0, 255, 100, 255});
            DrawText("BACKSPACE to delete", TOTALWIDTH/2-100, 306, 14, Color{120, 120, 140, 255});
            std::string hint = "(" + std::to_string(playerName.size()) + "/" + std::to_string(MAX_NAME_LENGTH) + ")";
            DrawText(hint.c_str(), TOTALWIDTH/2-25, 330, 13, Color{120, 120, 140, 255});
            DrawLine(60, 358, TOTALWIDTH-60, 358, Color{60, 60, 80, 255});
            Renderer::drawLeaderboard(leaderboard, TOTALWIDTH/2-80, 370);
            Renderer::drawExceptionBox(exceptionMsg, exceptionTimer);
            return;
        }

        board.draw();
        current.drawWithGhost(board);

        int px = COLS * CELL;
        DrawRectangle(px, 0, SIDEBARWIDTH, TOTALHEIGHT, Color{20, 20, 30, 255});
        DrawText("TETRIS", px+16, 12, 26, WHITE);
        DrawLine(px+10, 44, px+SIDEBARWIDTH-10, 44, Color{80, 80, 100, 255});

        DrawText("SCORE",  px+16, 52,  14, LIGHTGRAY);
        DrawText(TextFormat("%d", score), px+16, 68, 20, YELLOW);
        DrawText("LEVEL",  px+16, 100, 14, LIGHTGRAY);
        DrawText(TextFormat("%d", level), px+16, 116, 20, YELLOW);
        DrawText("LINES",  px+16, 148, 14, LIGHTGRAY);
        DrawText(TextFormat("%d", linesTotal), px+16, 164, 20, YELLOW);
        DrawText("PLAYER", px+16, 184, 12, LIGHTGRAY);
        DrawText(playerName.c_str(), px+16, 198, 13, Color{100, 220, 255, 255});
        DrawText("NEXT",   px+16, 216, 14, LIGHTGRAY);
        next.drawPreview(px+16, 234);

        DrawText(TextFormat("UNDO: %d/%d", undoStack.size(), MAX_UNDO), px+16, 290, 12, Color{150, 150, 170, 255});
        Renderer::drawLeaderboard(leaderboard, px+10, 315);

        DrawText("CONTROLS",          px+12, 445, 13, Color{120, 120, 140, 255});
        DrawText("Arrows: Move/Rot",  px+12, 461, 11, Color{150, 150, 170, 255});
        DrawText("SPACE: Hard drop",  px+12, 475, 11, Color{150, 150, 170, 255});
        DrawText("U: Undo  R: Reset", px+12, 489, 11, Color{150, 150, 170, 255});
        DrawText("M: Mute music",     px+12, 503, 11, Color{150, 150, 170, 255});

        if (gameOver) {
            DrawRectangle(0, TOTALHEIGHT/2-60, COLS*CELL, 120, Color{0, 0, 0, 195});
            DrawText("GAME OVER", 28, TOTALHEIGHT/2-40, 36, RED);
            DrawText(TextFormat("Score: %d", score), 28, TOTALHEIGHT/2+4, 22, WHITE);
            DrawText("R: Restart  U: Undo", 28, TOTALHEIGHT/2+34, 18, LIGHTGRAY);
        }

        Renderer::drawExceptionBox(exceptionMsg, exceptionTimer);
    }

    bool operator==(const Game& other) const {
        return score == other.score && linesTotal == other.linesTotal && board == other.board;
    }
    bool operator!=(const Game& other) const { return !(*this == other); }

    int getScore() const { return score; }
    int getLevel() const { return level; }
    bool isGameOver() const { return gameOver; }
    int getUndoCount() const { return undoStack.size(); }
    bool isBlocking() const { return exceptionBlocking; }
};

// ─────────────────────────────────────────────────────────────────────────────
int main() {
    srand((unsigned)time(nullptr));

    InitWindow(TOTALWIDTH, TOTALHEIGHT, "Tetris");
    SetTargetFPS(60);

    // Audio must be initialized before loading sounds
    // ChiptunePlayer calls InitAudioDevice() in init()
    ChiptunePlayer music;
    music.init();

    SFXBundle sfx;
    sfx.init();

    Game game(&sfx);

    bool muted = false;
    SetMasterVolume(1.0f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        
        if (IsKeyPressed(KEY_M)) {
            muted = !muted;
            SetMasterVolume(muted ? 0.0f : 1.0f);
        }

        music.update(dt);
        game.handleInput();
        game.update(dt);

        BeginDrawing();
        ClearBackground(Color{15, 15, 20, 255});
        game.draw();

        
        if (muted)
            DrawText("MUTED [M]", COLS*CELL + 16, TOTALHEIGHT - 22, 12, Color{200, 80, 80, 255});

        EndDrawing();
    }

    sfx.unload();
    music.stop();   
    CloseWindow();
    return 0;
}           