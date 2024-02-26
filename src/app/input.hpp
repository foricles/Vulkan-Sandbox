#pragma once
#include <vector>

enum class EKeys
{
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	TAB,
	ALT,
	CTRL,
	SHIFT,
	SPACE,
	ENTER,
	BACK,
	ESC,
	DEL,
	LARR,
	RARR,
	UARR,
	DARR,

	__SIZE__
};


class AppInput
{
public:
	AppInput()
		: key_pressed(uint32_t(EKeys::__SIZE__), false)
		, maped_keyboad()
	{
		maped_keyboad = {
			EKeys::__SIZE__,		// VK_LBUTTON	0x00
			EKeys::__SIZE__,		// VK_RBUTTON	0x01
			EKeys::__SIZE__,		// VK_RBUTTON	0x02
			EKeys::__SIZE__,		// VK_CANCEL	0x03
			EKeys::__SIZE__,		// VK_MBUTTON	0x04
			EKeys::__SIZE__,		// VK_XBUTTON1	0x05
			EKeys::__SIZE__,		// VK_XBUTTON2	0x06
			EKeys::__SIZE__,		// Undefined	0x07
			EKeys::BACK,			// VK_BACK		0x08
			EKeys::TAB,				// VK_TAB		0x09
			EKeys::__SIZE__,		// Reserved		0x0A
			EKeys::__SIZE__,		// Reserved		0x0B
			EKeys::__SIZE__,		// VK_CLEAR		0x0C
			EKeys::__SIZE__,		// VK_RETURN	0x0D
			EKeys::__SIZE__,		// Undefined	0x0E
			EKeys::__SIZE__,		// Undefined	0x0F
			EKeys::SHIFT,			// VK_SHIFT		0x10
			EKeys::__SIZE__,		// VK_MENU		0x12
			EKeys::__SIZE__,		// VK_PAUSE		0x13
			EKeys::__SIZE__,		// VK_CAPITAL	0x14
			EKeys::__SIZE__,		// VK_KANA		0x15
			EKeys::__SIZE__,		// VK_HANGUEL	0x15
			EKeys::__SIZE__,		// VK_HANGUL	0x15
			EKeys::__SIZE__,		// VK_IME_ON	0x16
			EKeys::__SIZE__,		// VK_JUNJA		0x17
			EKeys::__SIZE__,		// VK_FINAL		0x18
			EKeys::__SIZE__,		// VK_HANJA		0x19
			EKeys::__SIZE__,		// VK_IME_OFF	0x1A
			EKeys::ESC,				// VK_ESCAPE	0x1B
			EKeys::__SIZE__,		// VK_CONVERT	0x1C
			EKeys::__SIZE__,		// VK_NONCONVERT	0x1D
			EKeys::__SIZE__,		// VK_ACCEPT	0x1E
			EKeys::__SIZE__,		// VK_MODECHANGE	0x1F
			EKeys::SPACE,			// VK_SPACE		0x20
			EKeys::__SIZE__,		// VK_PRIOR		0x21
			EKeys::__SIZE__,		// VK_NEXT		0x22
			EKeys::__SIZE__,		// VK_END		0x23
			EKeys::__SIZE__,		// VK_HOME		0x24
			EKeys::LARR,			// VK_LEFT		0x25
			EKeys::UARR,			// VK_UP		0x26
			EKeys::RARR,			// VK_RIGHT		0x27
			EKeys::DARR,			// VK_DOWN		0x28
			EKeys::__SIZE__,		// VK_SELECT	0x29
			EKeys::__SIZE__,		// VK_PRINT		0x2A
			EKeys::__SIZE__,		// VK_EXECUTE	0x2B
			EKeys::__SIZE__,		// VK_SNAPSHOT	0x2C
			EKeys::__SIZE__,		// VK_INSERT	0x2D
			EKeys::DEL,				// VK_DELETE	0x2E
			EKeys::__SIZE__,		// VK_HELP		0x2F
			EKeys::K0,				// 0 key		0x30
			EKeys::K1,				// 1 key		0x31
			EKeys::K2,				// 2 key		0x32
			EKeys::K3,				// 3 key		0x33
			EKeys::K4,				// 4 key		0x34
			EKeys::K5,				// 5 key		0x35
			EKeys::K6,				// 6 key		0x36
			EKeys::K7,				// 7 key		0x37
			EKeys::K8,				// 8 key		0x38
			EKeys::K9,				// 9 key		0x39
			EKeys::__SIZE__,		// Undefined	0x3A
			EKeys::__SIZE__,		// Undefined	0x3B
			EKeys::__SIZE__,		// Undefined	0x3C
			EKeys::__SIZE__,		// Undefined	0x3D
			EKeys::__SIZE__,		// Undefined	0x3F
			EKeys::__SIZE__,		// Undefined	0x40
			EKeys::A,				// A key		0x41
			EKeys::B,				// B key		0x42
			EKeys::C,				// C key		0x43
			EKeys::D,				// D key		0x44
			EKeys::E,				// E key		0x45
			EKeys::F,				// F key		0x46
			EKeys::G,				// G key		0x47
			EKeys::H,				// H key		0x48
			EKeys::I,				// I key		0x49
			EKeys::J,				// J key		0x4A
			EKeys::K,				// K key		0x4B
			EKeys::L,				// L key		0x4C
			EKeys::M,				// M key		0x4D
			EKeys::N,				// N key		0x4E
			EKeys::O,				// O key		0x4F
			EKeys::P,				// P key		0x50
			EKeys::Q,				// Q key		0x51
			EKeys::R,				// R key		0x52
			EKeys::S,				// S key		0x53
			EKeys::T,				// T key		0x54
			EKeys::U,				// U key		0x55
			EKeys::V,				// V key		0x56
			EKeys::W,				// W key		0x57
			EKeys::X,				// X key		0x58
			EKeys::Y,				// Y key		0x59
			EKeys::Z,				// Z key		0x5A
			EKeys::__SIZE__,		// VK_LWIN		0x5B
			EKeys::__SIZE__,		// VK_RWIN		0x5C
			EKeys::__SIZE__,		// VK_APPS		0x5D
			EKeys::__SIZE__,		// Reserved		0x5E
			EKeys::__SIZE__,		// VK_SLEEP		0x5F
			EKeys::K0,				// VK_NUMPAD0	0x60
			EKeys::K1,				// VK_NUMPAD1	0x61
			EKeys::K2,				// VK_NUMPAD2	0x62
			EKeys::K3,				// VK_NUMPAD3	0x63
			EKeys::K4,				// VK_NUMPAD4	0x64
			EKeys::K5,				// VK_NUMPAD5	0x65
			EKeys::K6,				// VK_NUMPAD6	0x66
			EKeys::K7,				// VK_NUMPAD7	0x67
			EKeys::K8,				// VK_NUMPAD8	0x68
			EKeys::K9,				// VK_NUMPAD9	0x69	
			EKeys::__SIZE__,		// VK_MULTIPLY	0x6A
			EKeys::__SIZE__,		// VK_ADD		0x6B
			EKeys::__SIZE__,		// VK_SEPARATOR	0x6C
			EKeys::__SIZE__,		// VK_SUBTRACT	0x6D
			EKeys::__SIZE__,		// VK_DECIMAL	0x6E
			EKeys::__SIZE__,		// VK_DIVIDE	0x6F
			EKeys::F1,				// VK_F1		0x70
			EKeys::F2,				// VK_F2		0x71
			EKeys::F3,				// VK_F3		0x72
			EKeys::F4,				// VK_F4		0x73
			EKeys::F5,				// VK_F5		0x74
			EKeys::F6,				// VK_F6		0x75
			EKeys::F7,				// VK_F7		0x76
			EKeys::F8,				// VK_F8		0x77
			EKeys::F9,				// VK_F9		0x78
			EKeys::F10,				// VK_F10		0x79
			EKeys::F11,				// VK_F11		0x7A
			EKeys::F12,				// VK_F12		0x7B
			EKeys::__SIZE__,		// VK_F13		0x7C
			EKeys::__SIZE__,		// VK_F14		0x7D
			EKeys::__SIZE__,		// VK_F15		0x7E
			EKeys::__SIZE__,		// VK_F16		0x7F
			EKeys::__SIZE__,		// VK_F17		0x80
			EKeys::__SIZE__,		// VK_F18		0x81
			EKeys::__SIZE__,		// VK_F19		0x82
			EKeys::__SIZE__,		// VK_F20		0x83
			EKeys::__SIZE__,		// VK_F21		0x84
			EKeys::__SIZE__,		// VK_F22		0x85
			EKeys::__SIZE__,		// VK_F23		0x86
			EKeys::__SIZE__,		// VK_F24		0x87
			EKeys::__SIZE__,		// Unassigned	0x88
			EKeys::__SIZE__,		// Unassigned	0x89
			EKeys::__SIZE__,		// Unassigned	0x8A
			EKeys::__SIZE__,		// Unassigned	0x8B
			EKeys::__SIZE__,		// Unassigned	0x8C
			EKeys::__SIZE__,		// Unassigned	0x8D
			EKeys::__SIZE__,		// Unassigned	0x8E
			EKeys::__SIZE__,		// Unassigned	0x8F
			EKeys::__SIZE__,		// VK_NUMLOCK	0x90
			EKeys::__SIZE__,		// OEM specific	0x91
			EKeys::__SIZE__,		// OEM specific	0x92
			EKeys::__SIZE__,		// OEM specific	0x93
			EKeys::__SIZE__,		// OEM specific	0x94
			EKeys::__SIZE__,		// OEM specific	0x95
			EKeys::__SIZE__,		// OEM specific	0x96
			EKeys::__SIZE__,		// Unassigned	0x97
			EKeys::__SIZE__,		// Unassigned	0x98
			EKeys::__SIZE__,		// Unassigned	0x99
			EKeys::__SIZE__,		// Unassigned	0x9A
			EKeys::__SIZE__,		// Unassigned	0x9B
			EKeys::__SIZE__,		// Unassigned	0x9C
			EKeys::__SIZE__,		// Unassigned	0x9D
			EKeys::__SIZE__,		// Unassigned	0x9F
			EKeys::SHIFT,			// VK_LSHIFT	0xA0
			EKeys::SHIFT,			// VK_RSHIFT	0xA1
			EKeys::CTRL,			// VK_LCONTROL	0xA2
			EKeys::CTRL,			// VK_RCONTROL	0xA3
			EKeys::ALT,				// VK_LMENU		0xA4
			EKeys::ALT,				// VK_RMENU		0xA5	
		};
	}

	bool KeyPressed(EKeys keyCode) const
	{
		return key_pressed[uint32_t(keyCode)];
	}


	void OnKeyDown(uint32_t keycode)
	{
		if ((keycode < maped_keyboad.size()) && (maped_keyboad[keycode] != EKeys::__SIZE__))
		{
			key_pressed[size_t(maped_keyboad[keycode])] = true;
		}
	}

	void OnKeyUp(uint32_t keycode)
	{
		if ((keycode < maped_keyboad.size()) && (maped_keyboad[keycode] != EKeys::__SIZE__))
		{
			key_pressed[size_t(maped_keyboad[keycode])] = false;
		}
	}

private:
	std::vector<bool> key_pressed;
	std::vector<EKeys> maped_keyboad;
};