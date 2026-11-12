#include "lib.hpp"
#include <cstring>
#include <initializer_list>

/*
 * Let's Sing 2026 (base 0100EE4020D18000 + update v131072, build id 1C344DD17322D4FA3E8F50063340C7D0):
 * skip a long intro and the wait after the last note. In a normal game (InGameClassic) or a Feat game (InGameFeat);
 * never Fest/online or Career:
 *   - pressing ZR or ZL (SL/SR on a single Joy-Con) while more than 6 s are left before the first note jumps to 3 s
 *     before it;
 *   - holding one of them for 3 s after the last note jumps to 1 s before the end of the song: the game then ends
 *     the song itself (results and saved score as always).
 * The jumps use the game's own InGameSimple.SetTime, which moves the audio and the video; the lyrics follow the audio.
 * InGameFeat has no UpdateVx of its own (it runs InGameSimple's) and InGameClassic's does not call InGameSimple's,
 * so each one is hooked.
 * Addresses are offsets in the main module (Il2CppDumper RVAs) and only valid for this build.
 */

namespace offsets {
    constexpr uintptr_t InGameClassic_UpdateVx = 0x47A2E50;   // public override void UpdateVx()
    constexpr uintptr_t InGameSimple_UpdateVx = 0x47A4700;    // the one InGameFeat runs; ends the song at songEnd
    constexpr uintptr_t InGameSimple_SetTime = 0x47A9C20;     // protected override void SetTime(float time), seconds
    constexpr uintptr_t KaraokeManager_get_Instance = 0x45908B0;
    constexpr uintptr_t KaraokeManager_get_KTime = 0x4590970;  // song clock in seconds, follows the audio

    constexpr ptrdiff_t Object_klass = 0x0;
    constexpr ptrdiff_t Class_name = 0x10;
    constexpr ptrdiff_t InGame_songEnd = 0xA8;                // the song ends when karaokeTime passes it
    constexpr ptrdiff_t InGame_karaokeTime = 0xAC;
    constexpr ptrdiff_t KaraokeManager_KaraokeSong = 0x20;
    constexpr ptrdiff_t KaraokeSong_KaraokeNotes = 0x10;      // KaraokeNote[][] (one array per voice)
    constexpr ptrdiff_t KaraokeSong_KaraokeNotesAll = 0x18;   // KaraokeNote[][]: every voice (Feat parts too)
    constexpr ptrdiff_t Array_length = 0x18;
    constexpr ptrdiff_t Array_items = 0x20;
    constexpr ptrdiff_t KaraokeObject_StartTime = 0x10;
    constexpr ptrdiff_t KaraokeObject_EndTime = 0x14;
}

constexpr float MIN_LEFT = 6.0f;   // seconds before the first note for the skip to be allowed
constexpr float LEAD_IN = 3.0f;    // land this many seconds before the first note
constexpr float END_HOLD = 3.0f;   // seconds the button must stay held after the last note to end the song
constexpr float END_AFTER = 0.5f;  // the hold only counts this long after the last note ended
constexpr float END_LEAD = 1.0f;   // land this many seconds before the end of the song
constexpr float END_MIN = 2.0f;    // with less than this left the song is ending anyway

namespace nn::hid {
    struct NpadBaseState {
        s64 sampling_number;
        u64 buttons;
        s32 analog_stick_l[2];
        s32 analog_stick_r[2];
        u32 attributes;
        u32 reserved;
    };
    struct NpadFullKeyState : NpadBaseState {};
    struct NpadHandheldState : NpadBaseState {};
    struct NpadJoyDualState : NpadBaseState {};
    struct NpadJoyLeftState : NpadBaseState {};
    struct NpadJoyRightState : NpadBaseState {};

    void GetNpadState(NpadFullKeyState *, const u32 &);
    void GetNpadState(NpadHandheldState *, const u32 &);
    void GetNpadState(NpadJoyDualState *, const u32 &);
    void GetNpadState(NpadJoyLeftState *, const u32 &);
    void GetNpadState(NpadJoyRightState *, const u32 &);
}

namespace {
    constexpr u64 BUTTON_ZL = 1ull << 8, BUTTON_ZR = 1ull << 9;
    constexpr u64 BUTTON_SL_SR = (1ull << 24) | (1ull << 25) | (1ull << 26) | (1ull << 27);
    constexpr u64 SKIP_BUTTONS = BUTTON_ZL | BUTTON_ZR | BUTTON_SL_SR;

    using GetInstanceFn = void *(*)(void *method);
    using GetKTimeFn = float (*)(void *self, void *method);
    using SetTimeFn = void (*)(void *self, float time, void *method);

    template<typename T>
    T At(void *object, ptrdiff_t offset) {
        return *reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(object) + offset);
    }

    template<typename State>
    u64 Buttons(u32 id) {
        State state = {};
        nn::hid::GetNpadState(&state, id);
        return state.buttons;
    }

    bool SkipPressed() {
        u64 buttons = Buttons<nn::hid::NpadHandheldState>(0x20);
        for (u32 id = 0; id < 8; id++) {
            buttons |= Buttons<nn::hid::NpadFullKeyState>(id) | Buttons<nn::hid::NpadJoyDualState>(id)
                     | Buttons<nn::hid::NpadJoyLeftState>(id) | Buttons<nn::hid::NpadJoyRightState>(id);
        }
        return (buttons & SKIP_BUTTONS) != 0;
    }

    bool IsClassName(void *object, const char *name) {
        void *klass = At<void *>(object, offsets::Object_klass);
        const char *klass_name = klass ? At<const char *>(klass, offsets::Class_name) : nullptr;
        return klass_name && std::strcmp(klass_name, name) == 0;
    }

    /* Start of the first note and end of the last one, of any voice. False when the song has no notes yet. */
    bool NoteRange(void *manager, float *first, float *last) {
        void *song = At<void *>(manager, offsets::KaraokeManager_KaraokeSong);
        if (!song) {
            return false;
        }
        bool found = false;
        for (ptrdiff_t field : {offsets::KaraokeSong_KaraokeNotes, offsets::KaraokeSong_KaraokeNotesAll}) {
            void *voices = At<void *>(song, field);
            if (!voices) {
                continue;
            }
            uintptr_t count = At<uintptr_t>(voices, offsets::Array_length);
            for (uintptr_t i = 0; i < count && i < 16; i++) {
                void *notes = At<void *>(voices, offsets::Array_items + i * sizeof(void *));
                uintptr_t length = notes ? At<uintptr_t>(notes, offsets::Array_length) : 0;
                for (uintptr_t k = 0; k < length && k < 20000; k++) {
                    void *note = At<void *>(notes, offsets::Array_items + k * sizeof(void *));
                    if (!note) {
                        continue;
                    }
                    float start = At<float>(note, offsets::KaraokeObject_StartTime);
                    float end = At<float>(note, offsets::KaraokeObject_EndTime);
                    if (!found || start < *first) {
                        *first = start;
                    }
                    if (!found || end > *last) {
                        *last = end;
                    }
                    found = true;
                }
            }
        }
        return found;
    }

    bool s_WasPressed = true;  // a button held while the song starts does not count
    void *s_Screen = nullptr;
    float s_LastTime = 0.0f;
    float s_HeldSince = -1.0f;  // game clock when the hold after the last note began

    void Tick(void *self) {
        auto get_instance = reinterpret_cast<GetInstanceFn>(exl::util::modules::GetTargetOffset(
            offsets::KaraokeManager_get_Instance));
        auto get_ktime = reinterpret_cast<GetKTimeFn>(exl::util::modules::GetTargetOffset(
            offsets::KaraokeManager_get_KTime));
        void *manager = get_instance(nullptr);
        if (!manager) {
            return;
        }
        float now = get_ktime(manager, nullptr);
        if (self != s_Screen || now + 1.0f < s_LastTime) {  // another song, or the same one restarted (Retry)
            s_Screen = self;
            s_WasPressed = true;
            s_HeldSince = -1.0f;
        }
        s_LastTime = now;

        bool pressed = SkipPressed();
        bool just_pressed = pressed && !s_WasPressed;
        s_WasPressed = pressed;
        float first = 0.0f, last = 0.0f;
        if (!pressed || now < 0.0f || !NoteRange(manager, &first, &last)) {
            s_HeldSince = -1.0f;
            return;
        }
        auto set_time = reinterpret_cast<SetTimeFn>(exl::util::modules::GetTargetOffset(offsets::InGameSimple_SetTime));
        if (first > 0.0f && first - now >= MIN_LEFT) {  // the intro: one press
            s_HeldSince = -1.0f;
            if (just_pressed) {
                set_time(self, first - LEAD_IN, nullptr);
            }
            return;
        }
        float clock = At<float>(self, offsets::InGame_karaokeTime);
        float end = At<float>(self, offsets::InGame_songEnd);
        if (now < last + END_AFTER || end - clock < END_MIN) {  // still singing, or about to end
            s_HeldSince = -1.0f;
            return;
        }
        if (s_HeldSince < 0.0f || clock < s_HeldSince) {  // the wait after the last note: a held button
            s_HeldSince = clock;
        } else if (clock - s_HeldSince >= END_HOLD) {
            s_HeldSince = -1.0f;
            set_time(self, end - END_LEAD, nullptr);
        }
    }
}

HOOK_DEFINE_TRAMPOLINE(ClassicUpdate) {
    static void Callback(void *self, void *method) {
        Orig(self, method);
        if (self && IsClassName(self, "InGameClassic")) {  // not its subclasses: Fest (online), Career
            Tick(self);
        }
    }
};

HOOK_DEFINE_TRAMPOLINE(SimpleUpdate) {
    static void Callback(void *self, void *method) {
        bool feat = self && IsClassName(self, "InGameFeat");  // not Career's Feat and Duel, nor the singing class
        Orig(self, method);  // may leave the song (results): the song clock is read after it, as in Classic
        if (feat) {
            Tick(self);
        }
    }
};

extern "C" void exl_main(void *x0, void *x1) {
    exl::hook::Initialize();
    ClassicUpdate::InstallAtOffset(offsets::InGameClassic_UpdateVx);
    SimpleUpdate::InstallAtOffset(offsets::InGameSimple_UpdateVx);
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("Default exception handler called!");
}
