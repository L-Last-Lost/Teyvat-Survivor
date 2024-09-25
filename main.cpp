#include<graphics.h>
#include<string>
#include<vector>
#include <iostream>
//游戏窗口大小
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
//开始界面按钮大小
const int BUTTON_WIDTH = 192;
const int BUTTON_HEIGHT = 75;
//最大子弹数量
const int MAX_BULLET_SIZE = 8;
//游戏开始与运行
bool is_game_started = false;
bool running = true;
//渲染库
#pragma comment(lib, "MSIMG32.LIB")
#pragma comment(lib, "Winmm.lib")
//处理透明度使得主角周围的图形贴合背景
inline void putimage_alpha(int x, int y, IMAGE* img) {
	int w = img->getwidth();
	int h = img->getheight();
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA});
}
/// <summary>
/// 加载图片资源的类，这里的IMAGE是指针，因此可以用new来创建IMAGE对象
/// </summary>
class Atlas {
public:
	Atlas(LPCTSTR path, int num) {
		TCHAR path_file[256];
		for (size_t i = 0; i < num; i++)
		{
			_stprintf_s(path_file, path, i);

			IMAGE* frame = new IMAGE();
			loadimage(frame, path_file);
			frame_list.push_back(frame);
		}
	}
public:
	std::vector<IMAGE*> frame_list;
};
//预加载玩家和敌人图像
Atlas* atlas_player_left;
Atlas* atlas_player_right;
Atlas* atlas_enemy_left;
Atlas* atlas_enemy_right;

//序列帧动画效果
class Animation {
public:
	Animation(Atlas* atlas, int interval) {
		anim_atlas = atlas;
		interval_ms = interval;
	}
	~Animation() = default;

	void play(int x, int y, int delta) {
		timer += delta;
		if (timer >= interval_ms) {
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size();
			timer = 0;
		}

		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}
private:
	int timer = 0;  // 计时器
	int idx_frame = 0; // 帧索引
	int interval_ms = 0;  // 多长时间换一次帧图片，也就是图片的帧数

private:
	Atlas* anim_atlas;
};


//子弹类
class Bullet {
public:
	POINT position = { 0, 0 };

public:
	Bullet() = default;
	~Bullet() = default;

	void Draw() const {
		setlinecolor(RGB(255, 155, 50));
		setfillcolor(RGB(200, 75, 10));
		fillcircle(position.x, position.y, RADIUS);
	}

private:
	const int RADIUS = 10; //半径
};


//游戏开始按钮
class Button {
public:
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed) {
		region = rect;
		loadimage(&img_idle, path_img_idle);
		loadimage(&img_hovered, path_img_hovered);
		loadimage(&img_pushed, path_img_pushed);

	}
	~Button() = default;

	void Draw() {
		switch (status)
		{
		case Button::Status::Idle:
			putimage_alpha(region.left, region.top, &img_idle);
			break;
		case Button::Status::Hovered:
			putimage_alpha(region.left, region.top, &img_hovered);
			break;
		case Button::Status::Pushed:
			putimage_alpha(region.left, region.top, &img_pushed);
			break;
		default:
			break;
		}
	}

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message)
		{
		case WM_MOUSEMOVE:
			if (status == Status::Idle && CheckCursoHit(msg.x, msg.y))
				status = Status::Hovered;
			else if(status == Status::Hovered && !CheckCursoHit(msg.x, msg.y))
				status = Status::Idle;
			break;
		case WM_LBUTTONDOWN:
			if (CheckCursoHit(msg.x, msg.y))
				status = Status::Pushed;
			break;
		case WM_LBUTTONUP:
			if (status == Status::Pushed)
				OnClick();
			break;
		default:
			break;
		}
	}
protected:
	virtual void OnClick() = 0;

private:
	enum class Status {
		Idle = 0,
		Hovered,
		Pushed
	};
private:
	RECT region; //描述位置大小 
	IMAGE img_idle;
	IMAGE img_hovered;
	IMAGE img_pushed;
	Status status = Status::Idle;

private:
	bool CheckCursoHit(int x, int y) {
		return x >= region.left && x <= region.right && y >= region.top && y <= region.bottom;
	}
};

//游戏开始按钮
class StartGameButton : public Button {
public:
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~StartGameButton() = default;

protected:
	void OnClick() {
		is_game_started = true;

		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

// 退出游戏按钮
class QuitGameButton : public Button {
public:
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		:Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default;

protected:
	void OnClick() {
		running = true;
	}
};

// 玩家类
class Player {
public :
	Player() {
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		anim_left = new Animation(atlas_player_left,45);
		anim_right = new Animation(atlas_player_right, 45);

	}
	~Player() {
		delete anim_left;
		delete anim_right;
	}

	void ProcessEvent(const ExMessage& msg) {
		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = true;
				break;
			case VK_DOWN:
				is_move_down = true;
				break;
			case VK_LEFT:
				is_move_left = true;
				break;
			case VK_RIGHT:
				is_move_right = true;
				break;
			}
			break;

		case WM_KEYUP:
			switch (msg.vkcode) {
			case VK_UP:
				is_move_up = false;
				break;
			case VK_DOWN:
				is_move_down = false;
				break;
			case VK_LEFT:
				is_move_left = false;
				break;
			case VK_RIGHT:
				is_move_right = false;
				break;
			}
			break;
		}
	}

	POINT GetPosition() const {
		return this->player_pos;
	}

	int GetFRAME_WIDTH() const {
		return this->FRAME_WIDTH;
	}

	int GetFRAME_HIGHT() const {
		return this->FRAME_HIGHT;
	}

	void Move() {
		/*
			注意方向，上下左右对应的x和y是相反的
		*/
		int dir_x = is_move_right - is_move_left;
		int dir_y = is_move_down - is_move_up;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		//单位向量化，每次移动都是速度乘以方向
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			player_pos.x += (int)(SPEED * normalized_x);
			player_pos.y += (int)(SPEED * normalized_y);
		}

		//对玩家可以走的范围进行规范
		if (player_pos.x < 0) player_pos.x = 0;
		if (player_pos.y < 0) player_pos.y = 0;
		if (player_pos.x + FRAME_WIDTH > WINDOW_WIDTH) player_pos.x = WINDOW_WIDTH - FRAME_WIDTH;
		if (player_pos.y + FRAME_HIGHT > WINDOW_HEIGHT) player_pos.y = WINDOW_HEIGHT - FRAME_HIGHT;

	}

	void Draw(int delta) {
		int pos_shadow_x = player_pos.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = player_pos.y + FRAME_HIGHT - 8;

		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		static bool facing_left = false;
		int dir_x = is_move_right - is_move_left;
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;

		if (facing_left)
			anim_left->play(player_pos.x, player_pos.y, delta);
		else
			anim_right->play(player_pos.x, player_pos.y, delta);
	}

private:
	const int SPEED = 3;
	const int FRAME_WIDTH = 80;
	const int FRAME_HIGHT = 80;
	const int SHADOW_WIDTH = 32;
	
private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT player_pos = { 500, 500 };
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
};


// 敌人类
class Enemy {
public:
	Enemy() {
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_right, 45);

		enum class SpawnEdge {
			Up=0,
			Down,
			Left,
			Right
		};

		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::Up:
			position.x = rand() % WINDOW_WIDTH;
			position.y = -FRAME_HIGHT;
			break;
		case SpawnEdge::Down:
			position.x = rand() % WINDOW_WIDTH;
			position.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left:
			position.x = -FRAME_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right:
			position.x = WINDOW_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}
	~Enemy() {
		delete anim_left;
		delete anim_right;
	}

	//使用引用避免不必要的拷贝构造,const 限定符避免对这些内容进行更改

	bool CheckBulletCollision(const Bullet& bullet) {
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HIGHT;
		return is_overlap_x && is_overlap_y;
	}

	bool CheckPlayerCollision(const Player& player) {
		POINT check_position = { position.x + FRAME_WIDTH / 2 , position.y + FRAME_HIGHT / 2 };

		const POINT& player_position = player.GetPosition();
		int player_FRAME_WIDTH = player.GetFRAME_WIDTH();
		int player_FRAME_HIGHT = player.GetFRAME_HIGHT();

		bool is_overlap_x = check_position.x >= player_position.x && check_position.x <= player_position.x + player_FRAME_WIDTH;
		bool is_overlap_y = check_position.y >= player_position.y && check_position.y <= player_position.y + player_FRAME_HIGHT;
		return is_overlap_x && is_overlap_y;
	}

	void Move(const Player& player) {

		const POINT& player_position = player.GetPosition();
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		//单位向量化，每次移动都是速度乘以方向
		if (len_dir != 0) {
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
	}

	void Draw(int delta) {
		int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = position.y + FRAME_HIGHT - 35;
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		if (facing_left)
			anim_left->play(position.x, position.y, delta);
		else
			anim_right->play(position.x, position.y, delta);

		//std::cout << "Enemy position: (" << position.x << ", " << position.y << ")" << std::endl;

	}

	void Hurt() {
		alive = false;
	}
	
	bool CheckIsLive() {
		return alive;
	}

private:
	const int SPEED = 2;
	const int FRAME_WIDTH = 80;
	const int FRAME_HIGHT = 80;
	const int SHADOW_WIDTH = 48;

private:
	IMAGE img_shadow;
	Animation* anim_left;
	Animation* anim_right;
	POINT position = { 0, 0 };
	bool facing_left = false;
	bool alive = true;
};




void TryGenerateEnemy(std::vector<Enemy*>& enemy_list) {
	const int INTERVAL = 100;
	static int counter = 0;
	if ((++counter) % INTERVAL == 0) {
		//printf("hi\n");
		enemy_list.push_back(new Enemy());
	}
}

void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player) {
	const double RADIAL_SPPED = 0.0045;
	const double TANGENT_SPPED = 0.0055;
	double radian_interval = 2 * 3.14159 / bullet_list.size();
	POINT player_position = player.GetPosition();
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPPED);

	for (size_t i = 0; i < bullet_list.size(); i++)
	{
		double radian = GetTickCount() * TANGENT_SPPED + radian_interval * i;
		bullet_list[i].position.x = player_position.x + player.GetFRAME_WIDTH() / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.GetFRAME_HIGHT() / 2 + (int)(radius * cos(radian));

	}
}


void DrawPlayerScore(int score) {
	static TCHAR text[64];
	_stprintf_s(text, _T("当前玩家得分:%d"), score);

	setbkmode(TRANSPARENT);
	settextcolor(RGB(255, 85, 185));
	outtextxy(10, 10, text);
}

int main() {
	initgraph(1280, 720);

	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 6);


	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);

	int score = 0;
		
	Player player;
	ExMessage msg;
	IMAGE img_menu;
	IMAGE img_background;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullet> bullet_list(1);

	RECT region_btn_start_game, region_btn_quit_game;

	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));

	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_background, _T("img/background.png"));

	BeginBatchDraw();

	while (running) {
		DWORD start_time = GetTickCount();

		/*读取操作*/
		while (peekmessage(&msg)) {
			if (is_game_started)
				player.ProcessEvent(msg);
			else {
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}
		if (is_game_started) {
			//尝试生成敌人
			TryGenerateEnemy(enemy_list);
			//更新玩家位置
			player.Move();
			//更新子弹位置
			UpdateBullets(bullet_list, player);
			//更新敌人位置
			for (Enemy* enemy : enemy_list)
				enemy->Move(player);

			//检测子弹是否命中敌人
			for (Enemy* enemy : enemy_list) {
				for (const Bullet& bullet : bullet_list) {
					if (enemy->CheckBulletCollision(bullet)) {
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);
						enemy->Hurt();
						score++;
						if (bullet_list.size() < MAX_BULLET_SIZE && score % 5 == 0) {
							Bullet tmp;
							bullet_list.push_back(tmp);
						}
						break;
					}
				}
			}

			//删除被击中的敌人
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckIsLive()) {
					std::swap(enemy_list[i], enemy_list.back());
					enemy_list.pop_back();
					delete enemy;
				}
			}

			//检测是否死亡
			for (Enemy* enemy : enemy_list) {
				if (enemy->CheckPlayerCollision(player)) {
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分: %d !"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}
		}
		cleardevice();
		/*渲染游戏（画图）*/
		if (is_game_started) {
			putimage(0, 0, &img_background);

			player.Draw(1000 / 144);
			DrawPlayerScore(score);
			for (Bullet bullet : bullet_list)
				bullet.Draw();
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);
		}
		else {
			putimage(0, 0, &img_menu);
			btn_start_game.Draw();
			btn_quit_game.Draw();
		}
		FlushBatchDraw();

		// 避免资源浪费
		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 60) {
			Sleep(1000 / 60 - delta_time);
		}
	}
	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;

	EndBatchDraw();

	return 0;
}