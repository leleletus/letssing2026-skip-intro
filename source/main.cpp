#include "lib.hpp"
#include <cstring>

/*
 * Let's Sing 2026 (base 0100EE4020D18000 + update v131072, build id 1C344DD17322D4FA3E8F50063340C7D0):
 * skip a long intro. In a normal game (InGameClassic only: never Fest/online, Feat or Career), pressing ZR or ZL
 * (SL/SR on a single Joy-Con) while more than 6 s are left before the first note jumps to 3 s before it.
 * The jump uses the game's own InGameSimple.SetTime, which moves the audio and the video; the lyrics follow the audio.
 * Addresses are offsets in the main module (Il2CppDumper RVAs) and only valid for this build.
 */

namespace offsets {
    constexpr uintptr_t InGameClassic_UpdateVx = 0x47A2E50;   // public override void UpdateVx()
    constexpr uintptr_t InGameSimple_SetTime = 0x47A9C20;     // protected override void SetTime(float time), seconds
    constexpr uintptr_t KaraokeManager_get_Instance = 0x45908B0;
    constexpr uintptr_t KaraokeManager_get_KTime = 0x4590970;  // song clock in seconds, follows the audio

    constexpr ptrdiff_t Object_klass = 0x0;
    constexpr ptrdiff_t Class_name = 0x10;
    constexpr ptrdiff_t KaraokeManager_KaraokeSong = 0x20;
    constexpr ptrdiff_t KaraokeSong_KaraokeNotes = 0x10;      // KaraokeNote[][] (one array per voice)
    constexpr ptrdiff_t Array_length = 0x18;
    constexpr ptrdiff_t Array_items = 0x20;
    constexpr ptrdiff_t KaraokeObject_StartTime = 0x10;
}

constexpr float MIN_LEFT = 6.0f;   // seconds before the first note for the skip to be allowed
constexpr float LEAD_IN = 3.0f;    // land this many seconds before the first note

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

    /* Start of the first note of any voice, or a negative value when the song has no notes yet. */
    float FirstNoteTime(void *manager) {
        void *song = At<void *>(manager, offsets::KaraokeManager_KaraokeSong);
        void *voices = song ? At<void *>(song, offsets::KaraokeSong_KaraokeNotes) : nullptr;
        if (!voices) {
            return -1.0f;
        }
        float first = -1.0f;
        uintptr_t count = At<uintptr_t>(voices, offsets::Array_length);
        for (uintptr_t i = 0; i < count && i < 16; i++) {
            void *notes = At<void *>(voices, offsets::Array_items + i * sizeof(void *));
            if (!notes || At<uintptr_t>(notes, offsets::Array_length) == 0) {
                continue;
            }
            void *note = At<void *>(notes, offsets::Array_items);
            if (!note) {
                continue;
            }
            float start = At<float>(note, offsets::KaraokeObject_StartTime);
            if (first < 0.0f || start < first) {
                first = start;
            }
        }
        return first;
    }

    bool s_WasPressed = true;  // a button held while the song starts does not count
    void *s_Screen = nullptr;
    float s_LastTime = 0.0f;
}

HOOK_DEFINE_TRAMPOLINE(SkipIntro) {
    static void Callback(void *self, void *method) {
        Orig(self, method);
        if (!self || !IsClassName(self, "InGameClassic")) {
            return;
        }
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
        }
        s_LastTime = now;

        bool pressed = SkipPressed();
        bool just_pressed = pressed && !s_WasPressed;
        s_WasPressed = pressed;
        if (!just_pressed || now < 0.0f) {
            return;
        }
        float first = FirstNoteTime(manager);
        if (first <= 0.0f || first - now < MIN_LEFT) {
            return;
        }
        auto set_time = reinterpret_cast<SetTimeFn>(exl::util::modules::GetTargetOffset(offsets::InGameSimple_SetTime));
        set_time(self, first - LEAD_IN, nullptr);
    }
};

extern "C" void exl_main(void *x0, void *x1) {
    exl::hook::Initialize();
    SkipIntro::InstallAtOffset(offsets::InGameClassic_UpdateVx);
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("Default exception handler called!");
}
