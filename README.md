#  Falling Blocks Game

> **A classic block-stacking game built with raylib, OOP principles, and efficient data structures.**

[![Made with raylib](https://img.shields.io/badge/Made%20with-raylib-000000?style=flat&logo=raylib&logoColor=white)](https://www.raylib.com/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OOP](https://img.shields.io/badge/OOP-Design%20Patterns-green)](https://en.wikipedia.org/wiki/Object-oriented_programming)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

---

##  Screenshots

| Start Screen | Gameplay | Leaderboard | Game Over |
|--------------|----------|-------------|-----------|
| ![Start Screen](https://github.com/MuhammadBilalCS/Falling-Blocks-Game/blob/main/images/Start_Of_Game.png) | ![Gameplay](https://github.com/MuhammadBilalCS/Falling-Blocks-Game/blob/main/images/Falling%20Blocks.png) | ![Leaderboard](https://github.com/MuhammadBilalCS/Falling-Blocks-Game/blob/main/images/LeaderBoard.png) | ![Game Over](https://github.com/MuhammadBilalCS/Falling-Blocks-Game/blob/main/images/gameover.png) |

---

## 🎮 Game Features

-  **Classic Block-Stacking Mechanics** – 7 standard pieces (I, O, T, S, Z, J, L)
-  **Score & Level System** – Points increase with each line cleared
-  **Next Piece Preview** – See the upcoming piece
-  **Hold Piece** – Store a piece for later use
-  **Leaderboard** – Track your best scores
-  **Undo Feature** – Revert your last move
-  **Music & Sound** – Toggle mute with a single key
-  **Responsive Controls** – Keyboard and gamepad support

---

## ⌨️ Controls

| Key | Action |
|-----|--------|
| `↑` (Up Arrow) | Rotate piece |
| `↓` (Down Arrow) | Soft drop (increase fall speed) |
| `←` (Left Arrow) | Move left |
| `→` (Right Arrow) | Move right |
| `Space` | Hard drop (instant fall) |
| `Tab` | Sudden fall (drop to bottom) |
| `U` | Undo last move |
| `R` | Reset game |
| `M` | Mute/Unmute music |

---

##  Design Philosophy

### Object-Oriented Programming
- **Single Responsibility Principle** – Each class handles one concern
- **Open/Closed Principle** – New piece types can be added by extending the base class
- **Polymorphism** – Virtual functions for rotation, rendering, and collision
- **Encapsulation** – Each piece manages its own state

### Data Structures Used
| Structure | Purpose |
|-----------|---------|
| `std::vector<std::vector<int>>` | Game board grid (O(1) access) |
| `std::queue<Piece*>` | Next piece queue (FIFO) |
| `std::stack<Piece*>` | Hold piece mechanism (LIFO) |
| `std::unordered_map<int, int>` | Score table for cleared lines |
| `std::array<int, 7>` | Piece distribution weights |
| `std::vector<ScoreEntry>` | Leaderboard storage |

### Algorithms
- **Collision Detection** – AABB with grid alignment
- **Line Clearing** – Efficient row shifting with `std::rotate`
- **Rotation System** – SRS (Super Rotation System) implementation
- **Undo System** – State saving with Memento pattern

---

##  Built With

- **[raylib](https://www.raylib.com/)** – Simple and easy-to-use game development library
- **C++17** – Modern C++ with STL
- **OOP Design** – SOLID principles, inheritance, polymorphism
- **Data Structures** – Vectors, queues, stacks, and matrices
- **CMake** – Build system for cross-platform compilation


