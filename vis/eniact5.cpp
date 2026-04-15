#include <irrlicht.h>
#include <iostream>
#include <string>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <include/TiltFiveNative.h>
#include "COpenGLTexture.h"

using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

T5_Glasses glasses;
T5_GraphicsContextGL graphctxt;
double ipd;
double tiltx, tilty, tiltz;

ISceneManager *smgr;
ICursorControl *curs;
IGUIListBox *menu, *clocklist;
IGUIListBox *cards;
int menumode, menusel, nmenu;
IVolumeLightSceneNode *adneon[20][11];
IVolumeLightSceneNode *acneon[20][10];
IVolumeLightSceneNode *cycneon, *cycneon2;
IVolumeLightSceneNode *mpdneon[20], *mpsneon[10];
IVolumeLightSceneNode *fttenneon[3], *ftoneneon[3], *ftsetneon[3];
IVolumeLightSceneNode *ftaddneon[3], *ftsubneon[3], *ftringneon[3];
IVolumeLightSceneNode *consneon[20];
IVolumeLightSceneNode *mulsneon, *mulr1neon, *mulr3neon;
IVolumeLightSceneNode *dsqplneon, *dsqstat[30];
ICameraSceneNode *camera1, *camera2;
int laccmpos[9] = { 7725, 8335, 9555, 10165, 10775, 11385, 11995, 12605, 13215 };
int baccmpos[5] = { -2230, 210, 820, 1430, 2040 };
int raccmpos[6] = { 13690, 13080, 12470, 11860, 8810, 8200 };
wchar_t prompts[128][32];
char progs[128][32];

T5_Glasses
initglasses() {
	T5_Glasses glasses;
	T5_ClientInfo client;
	T5_Result r;
	T5_Context t5ctx;
	size_t len;
	char gbuf[256];

	client.applicationId = "ENIAC";
	client.applicationVersion = "0.0.2";
	client.sdkType = 0;
	client.reserved = 0;
	r = t5CreateContext(&t5ctx, &client, 0);
	if(r) {
		std::cerr << "Failed to create context" << std::endl;
	}
	len = 256;
	while(1) {
		r = t5ListGlasses(t5ctx, gbuf, &len);
		if(!r || r != T5_ERROR_NO_SERVICE)
			break;
		//std::cerr << ".";
	}
	r = t5CreateGlasses(t5ctx, gbuf, &glasses);
	if(r) {
		std::cerr << "Create glasses failed\n";
	}
	r = t5ReserveGlasses(glasses, "ENIAC");
	if(r) {
		std::cerr << "Reserve glasses failed\n";
	}
	while(1) {
		r = t5EnsureGlassesReady(glasses);
		if(!r)
			break;
		if(r != T5_ERROR_TRY_AGAIN) {
			std::cerr << "Glasse ready failed\n";
			return NULL;
		}
	}
	graphctxt.textureMode = kT5_GraphicsApi_GL_TextureMode_Pair;
	graphctxt.leftEyeArrayIndex = 0;
	graphctxt.rightEyeArrayIndex = 1;
	r = t5InitGlassesGraphicsContext(glasses, kT5_GraphicsApi_GL, &graphctxt);
	if(r) {
		std::cerr << "Init context failed\n";
	}
	r = t5GetGlassesFloatParam(glasses, 0, kT5_ParamGlasses_Float_IPD, &ipd);
	if(r) {
		std::cerr << "Get param " << r << std::endl;
	}

	return glasses;
}

void *
procwand(void *a) {
	T5_Result r;
	T5_WandStreamEvent event;
	unsigned long long lasttime;
	static int butta, buttb, buttx, butty, butt1, butt2, butt3;
	static int up, down;

	lasttime = 0LL;
	while(1) {
		r = t5ReadWandStreamForGlasses(glasses, &event, 100);
		if(r == T5_TIMEOUT)
			continue;
		if(r != T5_SUCCESS) {
			std::cerr << "Read wand failed\n";
			continue;
		}
		switch(event.type) {
		case kT5_WandStreamEventType_Connect:
			std::cerr << "Wand connect\n";
			break;
		case kT5_WandStreamEventType_Disconnect:
			std::cerr << "Wand disconnected\n";
			break;
		case kT5_WandStreamEventType_Desync:
			std::cerr << "Wand desynchronized\n";
			break;
		case kT5_WandStreamEventType_Report:
			if(event.report.analogValid) {
				if(event.report.stick.y > 0.9) {
					if(menumode) {
						if(up == 0) {
							if(menusel > 0) {
								menusel--;
								menu->setSelected(menusel);
							}
							up = 1;
						}
					}
					else {
						tiltz -= (event.report.timestampNanos - lasttime)
							* 0.000002;
					}
				}
				else if(event.report.stick.y < -0.9) {
					if(menumode) {
						if(down == 0) {
							if(menusel < nmenu - 1) {
								menusel++;
								menu->setSelected(menusel);
							}
							down = 1;
						}
					}
					else {
						tiltz += (event.report.timestampNanos - lasttime)
							* 0.000002;
					}
				}
				else {
					up = 0;
					down = 0;
				}
				if(event.report.stick.x > 0.9) {
					tiltx += (event.report.timestampNanos - lasttime) * 0.000002;
				}
				else if(event.report.stick.x < -0.9) {
					tiltx -= (event.report.timestampNanos - lasttime) * 0.000002;
				}
			}
			if(event.report.buttonsValid) {
				if(event.report.buttons.a) {
					if(butta == 0) {
						std::cout << "b r\n";
						butta = 1;
					}
				}
				else {
					butta = 0;
				}
				if(event.report.buttons.b) {
					if(buttb == 0) {
						std::cout << "b p\n";
						buttb = 1;
					}
				}
				else {
					buttb = 0;
				}
				if(event.report.buttons.x) {
					if(buttx == 0) {
						std::cout << "b c\n";
						buttx = 1;
					}
				}
				else {
					buttx = 0;
				}
				if(event.report.buttons.y) {
					if(butty == 0) {
						std::cout << "b i\n";
						butty = 1;
					}
				}
				else {
					butty = 0;
				}
				if(event.report.buttons.one) {
					if(butt1 == 0) {
						if(menumode == 1) {
							menu->setVisible(false);
							menumode = 0;
						}
						else if(menumode == 0) {
							menu->setSelected(0);
							menusel = 0;
							menu->setVisible(true);
							menumode = 1;
						}
						butt1 = 1;
					}
				}
				else {
					butt1 = 0;
				}
				if(event.report.buttons.two) {
					if(butt2 == 0) {
						if(menumode == 2) {
							clocklist->setVisible(false);
							menumode = 0;
						}
						else if(menumode == 0) {
							clocklist->setSelected(0);
							menusel = 0;
							clocklist->setVisible(true);
							menumode = 2;
						}
						butt2 = 1;
					}
				}
				else {
					butt2 = 0;
				}
				if(event.report.buttons.three) {
					if(butt3 == 0) {
						if(menumode) {
							std::cout << "R\n";
							std::cout << "l " << progs[menusel] << "\n";
							menumode = 0;
							menu->setVisible(false);
						}
						butt3 = 1;
					}
				}
				else {
					butt3 = 0;
				}
			}
			break;
		default:
			std::cerr << "Unknonwn wand event\n";
			break;
		}
		lasttime = event.report.timestampNanos;
	}
	return NULL;
}

void
initwand() {
	T5_Result r;
	T5_WandHandle hand[3];
	T5_WandStreamConfig config;
	unsigned char count;
	pthread_t wtid;

	count = 3;
	r = t5ListWandsForGlasses(glasses, hand, &count);
	if(r != T5_SUCCESS) {
		std::cerr << "List wands failed\n";
		return;
	}
	if(count == 0) {
		std::cerr << "No wand\n";
		return;
	}
	config.enabled = true;
	r = t5ConfigureWandStreamForGlasses(glasses, &config);
	if(r != T5_SUCCESS) {
		std::cerr << "Configure wand failed\n";
		return;
	}
	pthread_create(&wtid, NULL, procwand, NULL);
	t5SendImpulse(glasses, hand[0], 0.7, 200);
}

int
getpose(T5_FrameInfo *frameinfo) {
	T5_Result r;
	T5_GlassesPose pose;
	T5_Vec3 lpos, rpos;
	quaternion leye, reye;
	quaternion position, lrotpos, rrotpos, rotation1, rotation2;
	quaternion targgbd, upvec;
	vector3df targmodel;
	vector3df c1pos, c2pos;
	int i;
	static int cyc;

	i = 0;
	while(1) {
		r = t5GetGlassesPose(glasses,
			kT5_GlassesPoseUsage_GlassesPresentation, &pose);
		if(!r)
			break;
		if(i >= 20) {
			std::cerr << ".";
			return 0;
		}
		if(r != T5_ERROR_TRY_AGAIN)
			std::cerr << "Pose failed " << r << std::endl;
		i++;
	}
	frameinfo->texWidth_PIX = 1216;
	frameinfo->texHeight_PIX = 768;
	frameinfo->isSrgb = true;
	frameinfo->isUpsideDown = false;
	frameinfo->vci.startX_VCI = -0.794945419;
	frameinfo->vci.startY_VCI = -0.445228686;
	frameinfo->vci.width_VCI = 1.409890838;
	frameinfo->vci.height_VCI = 0.890457371;
	rotation1 = quaternion(pose.rotToGLS_GBD.x, pose.rotToGLS_GBD.y,
		pose.rotToGLS_GBD.z, pose.rotToGLS_GBD.w);
	rotation1.normalize();
	rotation2.W = rotation1.W;
	rotation2.X = -rotation1.X;
	rotation2.Y = -rotation1.Y;
	rotation2.Z = -rotation1.Z;
	leye = quaternion(-ipd / 2.0, 0.0, 0.0, 0.0);
	reye = quaternion(ipd / 2.0, 0.0, 0.0, 0.0);
	lrotpos = rotation1 * leye * rotation2;
	rrotpos = rotation1 * reye * rotation2;
	lpos.x = lrotpos.X + pose.posGLS_GBD.x;
	lpos.y = lrotpos.Y + pose.posGLS_GBD.y;
	lpos.z = lrotpos.Z + pose.posGLS_GBD.z;
	rpos.x = rrotpos.X + pose.posGLS_GBD.x;
	rpos.y = rrotpos.Y + pose.posGLS_GBD.y;
	rpos.z = rrotpos.Z + pose.posGLS_GBD.z;
	frameinfo->rotToLVC_GBD = pose.rotToGLS_GBD;
	frameinfo->posLVC_GBD = lpos;
	frameinfo->rotToRVC_GBD = pose.rotToGLS_GBD;
	frameinfo->posRVC_GBD = rpos;

	targgbd = rotation1 * quaternion(0.0, 0.0, -1.0, 0.0) * rotation2;
	targmodel = vector3df(12000.0 * targgbd.X - 600,
		12000.0 * targgbd.Z, 12000.0 * targgbd.Y);
	upvec = rotation1 * quaternion(0.0, 1.0, 0.0, 0.0) * rotation2;
	c1pos = c2pos = vector3df(12000.0 * pose.posGLS_GBD.x - 600,
		12000.0 * pose.posGLS_GBD.z - 1600,
		12000.0 * (pose.posGLS_GBD.y + 0.6));
	targmodel += c1pos;
	c1pos += vector3df(lrotpos.X + tiltx, lrotpos.Z + tiltz, lrotpos.Y);
	c2pos += vector3df(rrotpos.X + tiltx, rrotpos.Z + tiltz, rrotpos.Y);
	camera1->setPosition(c1pos);
	camera1->updateAbsolutePosition();
	camera1->setTarget(targmodel);
	camera1->setUpVector(vector3df(upvec.X, upvec.Z, upvec.Y));
	camera2->setPosition(c2pos);
	camera2->updateAbsolutePosition();
	camera2->setTarget(targmodel);
	camera2->setUpVector(vector3df(upvec.X, upvec.Z, upvec.Y));

	return 1;
}

void
setcams(vector3df pos, double angle) {
	camera1->setPosition(vector3df(pos.X - (30 * cos(angle)), 0,
		pos.Z + (30 * sin(angle))));
	camera1->updateAbsolutePosition();
	camera2->setPosition(vector3df(pos.X + (30 * cos(angle)), 0,
		pos.Z - (30 * sin(angle))));
	camera2->updateAbsolutePosition();
}

void
makemenu(IGUIEnvironment *guienv) {
	FILE *idx;
	IGUIFont *font;
	char rprompt[128][32];
	int i;
	char *p;
	wchar_t *q;

	idx = std::fopen("programs/index", "r");
	if(idx == NULL)
		return;
	nmenu = 0;
	while(1) {
		if(std::fscanf(idx, " %[^$]$%s", rprompt[nmenu], progs[nmenu]) < 2)
			break;
		p = rprompt[nmenu];
		q = prompts[nmenu];
		while(1) {
			*q = *p;
			if(*p == '\0')
				break;
			q++;
			p++;
		}
		nmenu++;
	}
	std::fclose(idx);
	font = guienv->getFont("vis/DejaVuSansMono.png");
	if(!font)
		std::cerr << "Cannot load font\n";
	else {
		guienv->getSkin()->setFont(font, EGDF_DEFAULT);
	}
	clocklist = guienv->addListBox(rect<s32>(500, 50, 800, 140), NULL, 1, true);
	clocklist->setItemHeight(30);
	clocklist->addItem(L"Continuous");
	clocklist->addItem(L"1 Add");
	clocklist->addItem(L"1 Pulse");
	clocklist->setVisible(false);
	menu = guienv->addListBox(rect<s32>(500, 50, 830, 50 + 30 * nmenu),
		NULL, 1, true);
	menu->setItemHeight(30);
	for(i = 0; i < nmenu; i++)
		menu->addItem(prompts[i]);
	menu->setVisible(false);
	cards = guienv->addListBox(rect<s32>(850, 900, 1680, 1050), NULL, 1, true);
}

class MyReceiver : public IEventReceiver {
public:
	MyReceiver() {}
	virtual bool OnEvent(const SEvent& event) {
		static vector3df campos(0,0,1000);
		static double angle = 0; 
		static vector3df forward(0,0,1);
		static vector3df target(0,0,20000);

		if(event.EventType == EET_MOUSE_INPUT_EVENT) {
			if(event.MouseInput.isRightPressed() && event.MouseInput.Shift) {
				curs->setPosition(-20, 0);
				return true;
			}
		}
		else if(event.EventType == EET_KEY_INPUT_EVENT) {
			if(!event.KeyInput.PressedDown)
				return false;
			else if(event.KeyInput.Key == KEY_UP || event.KeyInput.Char == 'u') {
				if(menumode == 1) {
					if(menusel > 0) {
						menusel--;
						menu->setSelected(menusel);
					}
				}
				else if(menumode == 2) {
					if(menusel > 0) {
						menusel--;
						clocklist->setSelected(menusel);
					}
				}
				else {
					campos += forward * 50;
					setcams(campos, angle);
				}
			}
			else if(event.KeyInput.Key == KEY_DOWN || event.KeyInput.Char == 'i') {
				if(menumode == 1) {
					if(menusel < nmenu - 1) {
						menusel++;
						menu->setSelected(menusel);
					}
				}
				else if(menumode == 2) {
					if(menusel < 2) {
						menusel++;
						clocklist->setSelected(menusel);
					}
				}
				else {
					campos -= forward * 50;
					setcams(campos, angle);
				}
			}
			else if(event.KeyInput.Key == KEY_LEFT || event.KeyInput.Char == 'y') {
				angle -= 0.02;
				target = vector3df(20000.0 * sin(angle), 0.0,
					20000.0 * cos(angle));
				forward = (target - campos).normalize();
				camera1->setTarget(target);
				camera2->setTarget(target);
				setcams(campos, angle);
			}
			else if(event.KeyInput.Key == KEY_RIGHT || event.KeyInput.Char == 'o') {
				angle += 0.02;
				target = vector3df(20000.0 * sin(angle), 0.0,
					20000.0 * cos(angle));
				forward = (target - campos).normalize();
				camera1->setTarget(target);
				camera2->setTarget(target);
				setcams(campos, angle);
			}
			else if(event.KeyInput.Char == 'L' || event.KeyInput.Char == 'q') {
				if(menumode == 1) {
					std::cout << "R\n" << std::flush;
					std::cout << "l " << progs[menusel] << "\n" << std::flush;
					menumode = 0;
					menu->setVisible(false);
				}
				else if(menumode == 2) {
					switch(menusel) {
					case 0:
						std::cout << "s cy.op co\n" << std::flush;
						break;
					case 1:
						std::cout << "s cy.op 1a\n" << std::flush;
						break;
					case 2:
						std::cout << "s cy.op 1p\n" << std::flush;
						break;
					}
					menumode = 0;
					clocklist->setVisible(false);
				}
			}
			else if(event.KeyInput.Char == 'M' || event.KeyInput.Char == 'w') {
				if(menumode == 1) {
					menu->setVisible(false);
					menumode = 0;
				}
				else if(menumode == 0) {
					menu->setSelected(0);
					menusel = 0;
					menu->setVisible(true);
					menumode = 1;
				}
			}
			else if(event.KeyInput.Char == 'C' || event.KeyInput.Char == 'r') {
				if(menumode == 2) {
					clocklist->setVisible(false);
					menumode = 0;
				}
				else if(menumode == 0) {
					clocklist->setSelected(0);
					menusel = 0;
					clocklist->setVisible(true);
					menumode = 2;
				}
			}
			else if(event.KeyInput.Char == 'd') {
				std::cout << "b c\n" << std::flush;
			}
			else if(event.KeyInput.Char == 'f') {
				std::cout << "b r\n" << std::flush;
			}
			else if(event.KeyInput.Char == 'a') {
				std::cout << "b i\n" << std::flush;
			}
			else if(event.KeyInput.Char == 's') {
				std::cout << "b p\n" << std::flush;
			}
			else if(event.KeyInput.Char == 'Q')
				exit(0);
		}
		return false;
	}
};

IVolumeLightSceneNode *
mkneon(double x, double y, double z) {
	return smgr->addVolumeLightSceneNode(0, -1, 32, 32,
		SColor(128, 255, 150, 0), SColor(128, 255, 150, 0),
		vector3df(x, y, z), vector3df(0.0, 0.0, 0.0), vector3df(8.0, 8.0, 8.0));
}

void
mvneon(IVolumeLightSceneNode *neon, double x, double y, double z) {
	neon->setPosition(vector3df(x, y, z));
}

void
mvdsqstat(int which, char dstat[], double x, double y, double z) {
	mvneon(dsqstat[which], x + 100 * (dstat[which] - '0'), y, z);
}

void *
stdinreader(void *a) {
	std::string msg;
	int unit, digit, val;
	int ms, mr1, mr3;
	float xpos, ystart, dir;
	char dstat[32];
	const char *cols;
	wchar_t card[81];
	int i;

	while(1) {
		std::getline(std::cin, msg);
		if(sscanf(msg.c_str(), "ad %d %d %d", &unit, &digit, &val) == 3) {
			if(unit < 9)
				mvneon(adneon[unit][digit], -2690, 390 + val * 35, 
					laccmpos[unit] + 47 * digit);
			else if(unit < 14)
				mvneon(adneon[unit][digit], baccmpos[unit-9] + 47 * digit,
					390 + val * 35, 14150);
			else
				mvneon(adneon[unit][digit], 2972, 390 + val * 35,
					raccmpos[unit-14] - 47 * digit);
		}
		else if(sscanf(msg.c_str(), "ac %d %d %d", &unit, &digit, &val) == 3) {
			if(unit < 9)
				mvneon(acneon[unit][digit], -2690, -400 + val * 627,
					laccmpos[unit] + 47 * digit);
			else if(unit < 14)
				mvneon(acneon[unit][digit], baccmpos[unit-9] + 47 * digit,
					-400 + val * 627, 14150);
			else
				mvneon(acneon[unit][digit], 2972, -400 + val * 627,
					raccmpos[unit-14] - 47 * digit);
		}
		else if(sscanf(msg.c_str(), "cy %d", &val) == 1) {
			val &= ~1;
			mvneon(cycneon, -2645, 300, 4732 + 9.6 * val);
			if(val <= 20)
				mvneon(cycneon2, -2645, 245, 4866);
			else if(val <= 36)
				mvneon(cycneon2, -2645, 245, 4965);
			else
				mvneon(cycneon2, -2800, 245, 4866);
		}
		else if(sscanf(msg.c_str(), "mpd %d %d", &digit, &val) == 2) {
			if(digit < 10) {
				mvneon(mpdneon[digit], -2745, 475 + 20 * val, 5368 + 40 * digit);
			}
			else {
				mvneon(mpdneon[digit], -2745, 475 + 20 * val,
					5970 + 40 * (digit - 10));
			}
		}
		else if(sscanf(msg.c_str(), "mps %d %d", &digit, &val) == 2) {
			if(digit < 5) {
				mvneon(mpsneon[digit], -2745, 80 + 20 * val, 5367 + 75 * digit);
			}
			else {
				mvneon(mpsneon[digit], -2745, 80 + 20 * val,
					5973 + 75 * (digit - 5));
			}
		}
		else if(sscanf(msg.c_str(), "ftar %d %d", &unit, &val) == 2) {
			switch(unit) {
			case 0:
				dir = 1;
				xpos = -2645;
				ystart = 6520;
				break;
			case 1:
				dir = -1;
				xpos = 2922;
				ystart = 11230;
				break;
			case 2:
				dir = -1;
				xpos = 2922;
				ystart = 10010;
				break;
			}
			mvneon(ftoneneon[unit], xpos, 300, ystart + (val / 10) * 19.2 * dir);
			mvneon(fttenneon[unit], xpos, 300,
				ystart + (val % 10) * 19.2 * dir + 250 * dir);
		}
		else if(sscanf(msg.c_str(), "ftr %d %d", &unit, &val) == 2) {
			switch(unit) {
			case 0:
				dir = 1;
				xpos = -2645;
				ystart = 6737;
				break;
			case 1:
				dir = -1;
				xpos = 2922;
				ystart = 11010;
				break;
			case 2:
				dir = -1;
				xpos = 2922;
				ystart = 9790;
				break;
			}
			val += 3;
			mvneon(ftringneon[unit], xpos, 245, ystart + val * 18.5 * dir);
		}
		else if(sscanf(msg.c_str(), "ftad %d %d", &unit, &val) == 2) {
			switch(unit) {
			case 0:
				xpos = -2745 + val * 100;
				ystart = 6612;
				break;
			case 1:
				xpos = 3022 - val * 100;
				ystart = 11138;
				break;
			case 2:
				xpos = 3022 - val * 100;
				ystart = 9918;
				break;
			}
			mvneon(ftaddneon[unit], xpos, 245, ystart);
		}
		else if(sscanf(msg.c_str(), "ftsu %d %d", &unit, &val) == 2) {
			switch(unit) {
			case 0:
				xpos = -2745 + val * 100;
				ystart = 6640;
				break;
			case 1:
				xpos = 3022 - val * 100;
				ystart = 11110;
				break;
			case 2:
				xpos = 3022 - val * 100;
				ystart = 9890;
				break;
			}
			mvneon(ftsubneon[unit], xpos, 245, ystart);
		}
		else if(sscanf(msg.c_str(), "ftse %d %d", &unit, &val) == 2) {
			switch(unit) {
			case 0:
				xpos = -2745 + val * 100;
				ystart = 6520;
				break;
			case 1:
				xpos = 3022 - val * 100;
				ystart = 11230;
				break;
			case 2:
				xpos = 3022 - val * 100;
				ystart = 10010;
				break;
			}
			mvneon(ftsubneon[unit], xpos, 245, ystart);
		}
		else if(sscanf(msg.c_str(), "ct %d %d", &digit, &val) == 2) {
			if(digit < 10) {
				mvneon(consneon[digit], 3095 - 100 * val, 660,
					7570 - 49 * (digit - 1));
			}
			else if(digit < 20) {
				mvneon(consneon[digit], 3095 - 100 * val, 204,
					7570 - 49 * (digit - 11));
			}
		}
		else if(sscanf(msg.c_str(), "m %d %*s %d %d", &ms, &mr1, &mr3) == 3) {
			mvneon(mulsneon,-910 + 20 * ms, 225, 14090);
			mvneon(mulr1neon, -1400, 230, 14190 - mr1 * 100);
			mvneon(mulr3neon, -180, 230, 14190 - mr1 * 100);
		}
		else if(sscanf(msg.c_str(), "d %d %*d %*s %s", &val, dstat) == 2) {
			mvneon(dsqplneon, -2720, 190 - val * 20, 8970);
			mvdsqstat(0, dstat, -2720, 370, 9080);
			mvdsqstat(1, dstat, -2720, 370, 9101);
			mvdsqstat(2, dstat, -2720, 370, 9122);
			mvdsqstat(3, dstat, -2720, 370, 9143);
			mvdsqstat(4, dstat, -2720, 370, 9164);
			mvdsqstat(5, dstat, -2720, 370, 9185);
			mvdsqstat(6, dstat, -2720, 370, 9206);
			mvdsqstat(7, dstat, -2720, 370, 9227);
			mvdsqstat(8, dstat, -2720, 370, 9238);
			mvdsqstat(9, dstat, -2720, 370, 9379);
			mvdsqstat(10, dstat, -2820, 190, 9010);
			mvdsqstat(11, dstat, -2820, 190, 9055);
			mvdsqstat(12, dstat, -2820, 190, 9100);
			mvdsqstat(13, dstat, -2820, 190, 9145);
			mvdsqstat(14, dstat, -2820, 190, 9190);
			mvdsqstat(15, dstat, -2820, 190, 9235);
			mvdsqstat(16, dstat, -2820, 190, 9280);
			mvdsqstat(17, dstat, -2820, 190, 9325);
			mvdsqstat(18, dstat, -2820, 190, 9370);
			mvdsqstat(19, dstat, -2820, 190, 9415);
			mvdsqstat(20, dstat, -2820, 165, 9010);
			mvdsqstat(21, dstat, -2820, 165, 9055);
			mvdsqstat(22, dstat, -2820, 165, 9100);
			mvdsqstat(23, dstat, -2820, 165, 9145);
			mvdsqstat(24, dstat, -2820, 165, 9190);
			mvdsqstat(25, dstat, -2820, 165, 9235);
			mvdsqstat(26, dstat, -2820, 165, 9280);
			mvdsqstat(27, dstat, -2820, 165, 9325);
			mvdsqstat(28, dstat, -2820, 165, 9370);
			mvdsqstat(29, dstat, -2820, 165, 9415);
		}
		else if(strncmp(msg.c_str(), "punch ", 6) == 0) {
			cols = msg.c_str() + 6;
			for(i = 0; i < 80; i++)
				card[i] = cols[i];
			i = cards->addItem(card);
			cards->setSelected(i);
		}
	}
}

void
makeneons(void) {
	int i, j;

	for(i = 0; i < 9; i++) {
		for(j = 0; j < 11; j++) {
			adneon[i][j] = mkneon(-2690, 390, laccmpos[i] + 47 * j);
		}
		for(j = 1; j <= 10; j++) {
			acneon[i][j] = mkneon(-2690, -400, laccmpos[i] + 47 * j);
		}
	}
	for(i = 0; i < 5; i++) {
		for(j = 0; j < 11; j++) {
			adneon[i+9][j] = mkneon(baccmpos[i] + 47 * j, 390, 14150);
		}
		for(j = 1; j <= 10; j++) {
			acneon[i+9][j] = mkneon(baccmpos[i] + 47 * j, -400, 14150);
		}
	}
	for(i = 0; i < 6; i++) {
		for(j = 0; j < 11; j++) {
			adneon[i+14][j] = mkneon(2972, 390, raccmpos[i] - 47 * j);
		}
		for(j = 1; j <= 10; j++) {
			acneon[i+14][j] = mkneon(2972, -400, raccmpos[i] - 47 * j);
		}
	}
	cycneon = mkneon(-2645, 280, 4732);
	cycneon2 = mkneon(-2645, 200, 4866);
	for(i = 0; i < 5; i++) {
		mpsneon[i] = mkneon(-2745, 80, 5367 + 75 * i);
		mpsneon[i+5] = mkneon(-2745, 80, 5973 + 75 * i);
	}
	for(i = 0; i < 10; i++) {
		mpdneon[i] = mkneon(-2745, 475, 5368 + 40 * i);
		mpdneon[i+10] = mkneon(-2745, 475, 5970 + 40 * i);
	}
	ftoneneon[0] = mkneon(-2645, 300, 6520);
	fttenneon[0] = mkneon(-2645, 300, 6770);
	ftsetneon[0] = mkneon(-2745, 245, 6520);
	ftaddneon[0] = mkneon(-2745, 245, 6612);
	ftsubneon[0] = mkneon(-2745, 245, 6640);
	ftringneon[0] = mkneon(-2645, 245, 6737);
	ftoneneon[1] = mkneon(2922, 300, 11230);
	fttenneon[1] = mkneon(2922, 300, 10980);
	ftsetneon[1] = mkneon(3022, 245, 11230);
	ftaddneon[1] = mkneon(3022, 245, 11138);
	ftsubneon[1] = mkneon(3022, 245, 11110);
	ftringneon[1] = mkneon(2922, 245, 11010);
	ftoneneon[2] = mkneon(2922, 300, 10010);
	fttenneon[2] = mkneon(2922, 300, 9760);
	ftsetneon[2] = mkneon(3022, 245, 10010);
	ftaddneon[2] = mkneon(3022, 245, 9918);
	ftsubneon[2] = mkneon(3022, 245, 9890);
	ftringneon[2] = mkneon(2922, 245, 9790);
	for(i = 0; i < 10; i++) {
		consneon[i] = mkneon(3095, 660, 7570 - 49 * i);
		consneon[i+10] = mkneon(3095, 204, 7570 - 49 * i);
	}
	mulsneon = mkneon(-910, 225, 14090);
	mulr1neon = mkneon(-1400, 230, 14190);
	mulr3neon = mkneon(-180, 230, 14190);
	dsqplneon = mkneon(-2720, 190, 8970);
	for(i = 0; i < 10; i++) {
		dsqstat[i] = mkneon(-2745, 370, 9080 + i * 21);
		dsqstat[i+10] = mkneon(-2820, 190, 9010 + i * 45);
		dsqstat[i+20] = mkneon(-2820, 165, 9010 + i * 45);
	}
}

int
main() {
	IrrlichtDevice *device;
	IVideoDriver *driver;
	IGUIEnvironment *guienv;
	IAnimatedMesh *mesh;
	IMeshSceneNode *node;
	ITexture *leftrtt, *rightrtt;
	COpenGLTexture *leftgl, *rightgl;
	ILightSceneNode *light[4];
	MyReceiver receiver;
	int i, j;
	T5_FrameInfo frameinfo;
	T5_Result r;
	intptr_t lefttex, righttex;
	vector3df targ;
	pthread_t tid;

	pthread_create(&tid, NULL, stdinreader, NULL);

	device = createDevice(video::EDT_OPENGL, dimension2d<u32>(1680,1050), 16,
		false, false, false, &receiver);
	if(device == NULL) {
		perror("create device");
		exit(1);
	}
	device->setWindowCaption(L"3D ENIAC");

	driver = device->getVideoDriver();
	smgr = device->getSceneManager();
	guienv = device->getGUIEnvironment();
	curs = device->getCursorControl();
	curs->setVisible(false);
	makemenu(guienv);

	mesh = smgr->getMesh("obj/eniact.obj");
	if(mesh == NULL) {
		perror("mesh");
		device->drop();
		exit(1);
	}
	node = smgr->addMeshSceneNode(mesh->getMesh(0));
	if(node == NULL) {
		perror("node");
		device->drop();
		exit(1);
	}
	node->setRotation(vector3df(-90, 180, 0));
	node->setPosition(vector3df(-2300, -1600, 4000));
	camera1 = smgr->addCameraSceneNode(0, vector3df(-50, 0, 1000),
		vector3df(0, 0, 9800));
	camera1->setFOV(1.0);
	camera1->bindTargetAndRotation(true);

	camera1->setFarValue(50000.0);
	camera1->setNearValue(1000.0);
	camera1->setAspectRatio(16.0/9.0);
	camera2 = smgr->addCameraSceneNode(0, vector3df(50, 0, 1000),
		vector3df(0, 0, 9800));
	camera2->setFOV(1.0);
	camera2->bindTargetAndRotation(true);
	camera2->setFarValue(50000.0);
	camera2->setNearValue(1000.0);
	camera2->setAspectRatio(16.0/9.0);
	light[0] = smgr->addLightSceneNode(0, vector3df(-1300, 2000, 9000),
		SColorf(0.6, 0.6, 0.6), 10000.0);
	light[0]->setLightType(ELT_POINT);
	light[0]->setVisible(true);
	light[1] = smgr->addLightSceneNode(0, vector3df(-1300, 2000, 3000),
		SColorf(0.6, 0.6, 0.6), 10000.0);
	light[1]->setLightType(ELT_POINT);
	light[1]->setVisible(true);
	light[2] = smgr->addLightSceneNode(0, vector3df(1300, 2000, 9000),
		SColorf(0.6, 0.6, 0.6), 10000.0);
	light[2]->setLightType(ELT_POINT);
	light[2]->setVisible(true);
	light[3] = smgr->addLightSceneNode(0, vector3df(1300, 2000, 3000),
		SColorf(0.6, 0.6, 0.6), 10000.0);
	light[3]->setLightType(ELT_POINT);
	light[3]->setVisible(true);

	makeneons();

	glasses = initglasses();
	initwand();

	leftrtt = driver->addRenderTargetTexture(dimension2d<u32>(1216, 768),
		"Left");
	rightrtt = driver->addRenderTargetTexture(dimension2d<u32>(1216, 768),
		"Right");

	leftgl = static_cast<COpenGLTexture*>(leftrtt);
	rightgl = static_cast<COpenGLTexture*>(rightrtt);
	lefttex = leftgl->getOpenGLTextureName();
	righttex = rightgl->getOpenGLTextureName();
	std::cerr << "left " << lefttex << " right " << righttex << std::endl;

	std::cout << "ready\n" << std::flush;

	while(device->run()) {
		driver->beginScene(true, true, SColor(255, 105, 110, 130));

		if(getpose(&frameinfo)) {
			frameinfo.leftTexHandle = (void *)lefttex;
			frameinfo.rightTexHandle = (void *)righttex;
			driver->setRenderTarget(leftrtt, true, true,
				SColor(255, 105, 110, 130));
			smgr->setActiveCamera(camera1);
			driver->setViewPort(rect<s32>(0, -200, 1216, 768));
			smgr->drawAll();
			menu->setRelativePosition(rect<s32>(515, 50, 845, 50 + 30 * nmenu));
			clocklist->setRelativePosition(rect<s32>(515, 50, 830, 140));
			cards->setRelativePosition(rect<s32>(400, 400, 1200, 550));
			guienv->drawAll();
			driver->setRenderTarget(rightrtt, true, true,
				SColor(255, 105, 110, 130));
			smgr->setActiveCamera(camera2);
			driver->setViewPort(rect<s32>(0, -200, 1216, 768));
			smgr->drawAll();
			menu->setRelativePosition(rect<s32>(500, 50, 830, 50 + 30 * nmenu));
			clocklist->setRelativePosition(rect<s32>(500, 50, 800, 140));
			cards->setRelativePosition(rect<s32>(350, 400, 1150, 550));
			guienv->drawAll();

			r = t5SendFrameToGlasses(glasses, &frameinfo);
			if(r) {
				std::cerr << "send frame failed " << r << std::endl;
			}

			driver->setRenderTarget(0, true, true, SColor(255, 105, 110, 130));
			driver->setViewPort(rect<s32>(0, 0, 1216, 768));
			smgr->drawAll();
			guienv->drawAll();

			//std::cerr << "/";
		}

		driver->endScene();
		device->yield();
	}
	device->drop();
}
