#include <graphics.h>   // EasyX库 from EasyX graphics library
#include <conio.h>
#include <time.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define RGBA(r, g, b, a) ((a) << 24 | (r) << 16 | (g) << 8 | (b))   // 由于这是旧版的EasyX库，得手动实现 RGBA 宏 Define RGBA

// 游戏常量 Game variables
const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 720;
const int BULLET_NUM = 50;
const int ENEMY_NUM = 10;
const int ENEMY_SPEED = 2;          // 敌机移动速度 enemy' spaceship speed
const int ENEMY_SPAWN_INTERVAL = 1500; // 敌机生成间隔 enemy'spaceship spawn interval 
const int MAX_PLAYER_HEALTH = 15;    // 最大血量 player's spaceship health
const int WIN_SCORE = 50;            // 胜利所需积分 target score
const int GAME_DURATION = 120;       // 游戏时间：2分钟（180秒）Game duration: 2 minutes (180 seconds)

// 枚举定义 define enemy
enum EnemyType {
    SMALL_ENEMY,  // 小敌机 small enemy' spaceship
    BIG_ENEMY     // 大敌机 big enemy's spaceship
};

// 游戏对象结构 
struct GameObject {
    int x, y;
    int width, height;
    bool active;
    int health;
    EnemyType type;  // 类型标识
};

// 全局资源 define global resources
IMAGE imgPlayer, imgBullet, imgEnemy[2], bg;
GameObject player, bullets[BULLET_NUM], enemies[ENEMY_NUM];
int score = 0;                       // 当前积分 current score
int gameTime = GAME_DURATION;        // 剩余游戏时间 remaining game duration
bool gameOver = false;               // 游戏是否结束 define game over/loss
bool gameWon = false;                // 游戏是否胜利 define game won

// 加载资源 load resources (images and musics)
void loadResources() {
    loadimage(&imgPlayer, _T("plane.png"));
    loadimage(&imgBullet, _T("bullet1_2.png"));
    loadimage(&imgEnemy[0], _T("enemy1_3.png"));  // 小敌机
    loadimage(&imgEnemy[1], _T("enemy2_3.png"));  // 大敌机
    loadimage(&bg, _T("background.jpg"));

    // 播放背景音乐（循环播放）play background music (loop)
    mciSendString(_T("open Space_Odyssey.mp3 alias bgm"), NULL, 0, NULL);
    mciSendString(_T("play bgm repeat"), NULL, 0, NULL);
}

// 初始化游戏 game initialization
void initGame() {
    player = {
        SCREEN_WIDTH / 2 - imgPlayer.getwidth() / 2,
        SCREEN_HEIGHT - imgPlayer.getheight() - 20,
        imgPlayer.getwidth(),
        imgPlayer.getheight(),
        true,
        MAX_PLAYER_HEALTH  // 使用常量初始化 initialize player's spaceship
    };

    for (auto& b : bullets) b.active = false;   // 初始化子弹 initialize bullets
    for (auto& e : enemies) e.active = false;   // 初始化敌机 initialize enemy' spaceship
    score = 0;                                  // 重置积分 reset score
    gameTime = GAME_DURATION;                  // 重置时间 reset time
    gameOver = false;                          // 重置游戏状态 reset game condition
    gameWon = false;                           // 重置胜利状态 

    // 重新播放背景音乐 replay background music
    mciSendString(_T("play bgm repeat"), NULL, 0, NULL);
}

// 绘制积分 visualize/display score
void drawScore() {
    TCHAR scoreText[32];
    _stprintf_s(scoreText, _T("SCORE: %d"), score);

    settextcolor(YELLOW);
    setbkmode(TRANSPARENT);
    settextstyle(40, 0, _T("Consolas"));
    outtextxy(20, 20, scoreText);  // 左上角显示积分 display the score on up left corner of the game interface
}

// 绘制剩余时间 visualize remaining game duration
void drawTime() {
    TCHAR timeText[32];
    int minutes = gameTime / 60;
    int seconds = gameTime % 60;
    _stprintf_s(timeText, _T("Time: %02d:%02d"), minutes, seconds);

    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(SCREEN_WIDTH - 150, 20, timeText);  // 右上角显示时间 display the remaining game duration on up right corner of the game interface
}

// 绘制游戏结束界面 visualize game end interface
void drawGameOver() {
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(40, 0, _T("Consolas"));

    if (gameWon) {
        outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, _T("You Win!"));
    }
    else {
        outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, _T("Game Over!"));
    }

    settextstyle(20, 0, _T("Consolas"));
    outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 20, _T("Press R to Restart"));
    outtextxy(SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, _T("Press Q to Quit"));

    if (gameOver) {
        mciSendString(_T("stop bgm"), NULL, 0, NULL);
    }
}

// 绘制血条函数 visualize player's spaceship health 
void drawHealthBar() {
    const int BAR_WIDTH = 200;      // 加宽血条 health bar width
    const int BAR_HEIGHT = 20;      // 加高血条 health bar height
    const int MARGIN = 20;          // 屏幕边距 
    const int TEXT_OFFSET = 5;      // 文字与血条的间距 distance between text and bar
    static float smoothHealth = MAX_PLAYER_HEALTH;  //

    // 计算位置（右下角）display the player's spaceship health on bottom right corner of the game interface
    int barX = SCREEN_WIDTH - BAR_WIDTH - MARGIN;
    int barY = SCREEN_HEIGHT - BAR_HEIGHT - MARGIN;

    // 绘制血量数值 draw the player's spaceship health in text
    TCHAR healthText[32];
    _stprintf_s(healthText, _T("HP: %d/%d"), player.health, MAX_PLAYER_HEALTH);

    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(barX, barY - 25, healthText);

    // 绘制背景条 
    setfillcolor(BLACK);  // 半透明红色
    solidroundrect(barX, barY, barX + BAR_WIDTH, barY + BAR_HEIGHT, 5, 5);

    // 绘制当前血量 draw current player's spaceship health
    smoothHealth += (player.health - smoothHealth) * 0.1f;
    float healthPercent = smoothHealth / MAX_PLAYER_HEALTH;
    //setfillcolor(RGBA(0, 255, 0, 150));  // 半透明绿色
    
    if (healthPercent < 0.3f) {
        setfillcolor(RGBA(0, 0, 255, 150));  // 变为红色
    }
    else {
        setfillcolor(RGBA(0, 255, 0, 150));
    }

    solidroundrect(barX, barY,
        barX + (int)(BAR_WIDTH * healthPercent),
        barY + BAR_HEIGHT, 5, 5);

    // 绘制边框 draw frame
    setlinecolor(WHITE);
    roundrect(barX, barY, barX + BAR_WIDTH, barY + BAR_HEIGHT, 5, 5);
}

// 碰撞检测判定 determine collision
bool checkCollision(const GameObject& a, const GameObject& b) {
    return a.x < b.x + b.width &&
        a.x + a.width > b.x &&
        a.y < b.y + b.height &&
        a.y + a.height > b.y;
}

// 添加Alpha混合绘制函数 Add in alpha-blending algorithm (for PNG images) 
void putimage_alpha(int x, int y, IMAGE* img) {
    DWORD* dst = GetImageBuffer();          // 获取窗口显存
    DWORD* src = GetImageBuffer(img);       // 获取图片显存
    int img_w = img->getwidth();
    int img_h = img->getheight();
    int win_w = getwidth();

    for (int iy = 0; iy < img_h; iy++) {
        for (int ix = 0; ix < img_w; ix++) {
            int pos = (y + iy) * win_w + (x + ix);
            if (pos >= 0 && pos < win_w * getheight()) {
                BYTE a = (src[iy * img_w + ix] >> 24) & 0xff; // 获取Alpha通道
                if (a != 0) {
                    dst[pos] = src[iy * img_w + ix];
                }
            }
        }
    }
}

int main() {
    initgraph(SCREEN_WIDTH, SCREEN_HEIGHT, EW_SHOWCONSOLE);
    loadResources();
    initGame();

    BeginBatchDraw();
    DWORD lastTime = GetTickCount();
    while (true) {
        cleardevice();

        if (!gameOver) {
            // 更新游戏时间 reset game duration
            DWORD currentTime = GetTickCount();
            if (currentTime - lastTime >= 1000) {
                gameTime--;
                lastTime = currentTime;
            }

            // 玩家移动 player's spaceship movement
            if (_kbhit()) {
                int key = _getch();
                switch (toupper(key)) {
                case 'W': if (player.y > 0) player.y -= 5; break;
                case 'S': if (player.y < SCREEN_HEIGHT - player.height) player.y += 5; break;
                case 'A': if (player.x > 0) player.x -= 5; break;
                case 'D': if (player.x < SCREEN_WIDTH - player.width) player.x += 5; break;
                case 'J':
                    for (auto& b : bullets) {
                        if (!b.active) {
                            b = {
                                player.x + player.width / 2 - imgBullet.getwidth() / 2,
                                player.y,
                                imgBullet.getwidth(),
                                imgBullet.getheight(),
                                true

                           
                            };
                            
                            // 播放子弹音效 play shooting bullet sound effect
                            mciSendString(_T("open shoot.mp3 alias shoot"), NULL, 0, NULL);
                            mciSendString(_T("play shoot from 0"), NULL, 0, NULL);
                            
                            break;
                        }
                    }
                    break;
                }
            }

            // 更新子弹 reset bullets
            for (auto& b : bullets) {
                if (b.active) {
                    b.y -= 10;
                    if (b.y < -b.height) b.active = false;
                }
            }

            // 生成敌机 spawn enemy' spaceship
            static DWORD lastEnemyTime = 0;
            if (GetTickCount() - lastEnemyTime > ENEMY_SPAWN_INTERVAL) {    
                for (auto& e : enemies) {
                    if (!e.active) {
                        // 敌机生成代码
                        bool isBig = (rand() % 5 == 0);
                        e = {
                            rand() % (SCREEN_WIDTH - imgEnemy[isBig].getwidth()),
                            -imgEnemy[isBig].getheight(),
                            imgEnemy[isBig].getwidth(),
                            imgEnemy[isBig].getheight(),
                            true,
                            isBig ? 3 : 1,
                            isBig ? BIG_ENEMY : SMALL_ENEMY  // 设置类型 set type of enemy' spaceship
                        };
                        lastEnemyTime = GetTickCount();
                        break;
                    }
                }
            }

            // 更新敌机 renew enemy' spaceship
            for (auto& e : enemies) {
                if (e.active) {
                    e.y += ENEMY_SPEED;  // 使用敌机速度 set enemy' spaceship speed
                    if (e.y > SCREEN_HEIGHT) e.active = false;
                }
            }

            // 碰撞检测 detect collision
            for (auto& e : enemies) {
                if (e.active && checkCollision(player, e)) {
                    player.health--;
                    e.active = false;
                    if (player.health <= 0) {
                        gameOver = true;
                    }
                }

                for (auto& b : bullets) {
                    if (b.active && e.active && checkCollision(b, e)) {
                        b.active = false;
                        if (--e.health <= 0) {
                            e.active = false;

                            // 播放敌机爆炸音效 play enemy' spaceship explosion sound effect
                            mciSendString(_T("open explosion.mp3 alias explosion"), NULL, 0, NULL);
                            mciSendString(_T("play explosion from 0"), NULL, 0, NULL);

                            // 积分计算 score counting
                            score += (e.type == BIG_ENEMY) ? 5 : 1;  // 根据敌机类型加分 count score according to the enemy' spaceship types
                            if (score >= WIN_SCORE) {
                                gameWon = true;
                                gameOver = true;
                            }
                        }
                    }
                }
            }

            // 检查游戏时间 check game duration
            if (gameTime <= 0) {
                gameOver = true;
            }

        
            putimage(0, 0, &bg);    // 绘制背景图 display background image
            putimage_alpha(player.x, player.y, &imgPlayer); // 绘制玩家飞船 display player's spaceship
            drawHealthBar();  // 绘制血条 display player's spaceship health 
            drawScore();       // 绘制积分 display score
            drawTime();        // 绘制剩余时间 display remaining game duration

            // 绘制子弹和敌机 display bullets and enemy' spaceship
            for (const auto& b : bullets) {
                if (b.active) putimage_alpha(b.x, b.y, &imgBullet);
            }
            for (const auto& e : enemies) {
                if (e.active) putimage_alpha(e.x, e.y, &imgEnemy[e.health > 1 ? 1 : 0]);
            }
        }
        else {
            // 游戏结束界面 display game end interface 
            drawGameOver();
            if (_kbhit()) {
                int key = _getch();
                if (toupper(key) == 'R') {
                    initGame();  // 重新开始游戏 restart game
                }
                else if (toupper(key) == 'Q') {
                    break;  // 退出游戏 quit game
                }
            }
        }

        FlushBatchDraw();
        Sleep(16);
    }
    EndBatchDraw();
    closegraph();
    return 0;
}