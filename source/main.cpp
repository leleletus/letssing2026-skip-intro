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
 *
 * Random song (v3): in the song list (KaraokeHub, SongsScreen), pressing ZL and ZR together starts a random song,
 * as if it had been chosen with A: the game goes on to the player selection with that song, and B comes back to
 * the list. Where the song comes from depends on the order of the list (v4): in Recommended, Language, Year, Theme
 * and Genre, from the row the cursor is in ("For you", "Top 10", "All songs", a language, a year...); in Title,
 * Artist and Favorites the rows are only letters, so from all the songs listed. Coming back with B, the game puts
 * the cursor on that song in the first row that has it, which may be another row ("All songs" when "For you" was
 * made again without it): while the cursor is still on the song the mod started, the next one comes from the row
 * that song was taken from. The mod does what
 * SongsScreen.UpdateVx does when a cover is validated: SuggestionsManager.CurrentSuggestions (the suggestion list the
 * song came from, or null), GameManager.CurrentGameSong = song, GameManager.GoTo("PlayerSelection4P"). Locked songs
 * and songs whose data is not on the console are never picked. The category is read when the first of the two
 * buttons goes down, before the game sees it, so whatever that button does alone in the list changes nothing.
 * Addresses are offsets in the main module (Il2CppDumper RVAs) and only valid for this build.
 */

namespace offsets {
    constexpr uintptr_t InGameClassic_UpdateVx = 0x47A2E50;   // public override void UpdateVx()
    constexpr uintptr_t InGameSimple_UpdateVx = 0x47A4700;    // the one InGameFeat runs; ends the song at songEnd
    constexpr uintptr_t InGameSimple_SetTime = 0x47A9C20;     // protected override void SetTime(float time), seconds
    constexpr uintptr_t KaraokeManager_get_Instance = 0x45908B0;
    constexpr uintptr_t KaraokeManager_get_KTime = 0x4590970;  // song clock in seconds, follows the audio

    constexpr uintptr_t SongsScreen_UpdateVx = 0x46014C0;     // song list of the KaraokeHub, every frame
    constexpr uintptr_t GameManager_get_Instance = 0x4580350;
    constexpr uintptr_t SortManager_get_Instance = 0x45A5D80;
    constexpr uintptr_t SortManager_get_CurrentSortType = 0x45A62D0;  // 0 = Recommended
    constexpr uintptr_t SuggestionsManager_get_Instance = 0x45A8630;
    constexpr uintptr_t PlaylistManager_get_Instance = 0x46A0920;
    constexpr uintptr_t PlaylistManager_get_IsInPlaylist = 0x46A0150;  // editing a playlist: A adds songs to it
    constexpr uintptr_t GameSong_get_IsLocked = 0x477B9F0;
    constexpr uintptr_t GameSong_get_CanRetrieveData = 0x477B970;
    constexpr uintptr_t InitializeRuntimeMetadata = 0x75BE00;   // il2cpp_codegen_initialize_runtime_metadata(slot *)
    constexpr uintptr_t WriteBarrier = 0x75BD30;                // Il2CppCodeGenWriteBarrier(field *, value)
    constexpr uintptr_t String_PlayerSelection4P = 0x85204E0;   // metadata slot of the scene name (a string literal)

    constexpr ptrdiff_t SongsScreen_songsGrid = 0xF8;
    constexpr ptrdiff_t SongsGrid_state = 0x2C;               // 0: covers, 1: details of a cover
    constexpr uintptr_t GridSongCover_get_Song = 0x45EB4F0;
    constexpr ptrdiff_t SongsGrid_currentCover = 0x120;       // GridSongCover under the cursor
    constexpr ptrdiff_t BaseHubCategory_Name = 0x20;          // string
    constexpr ptrdiff_t String_length = 0x10;
    constexpr ptrdiff_t String_chars = 0x14;                  // UTF-16
    constexpr ptrdiff_t SongsGrid_CurrentCategory = 0x130;    // GridSongCategory
    constexpr ptrdiff_t SongsGrid_Categories = 0x148;         // GridSongCategory[]: every row of the list
    constexpr ptrdiff_t GridSongCategory_Category = 0xD0;     // BaseHubCategory
    constexpr ptrdiff_t BaseHubCategory_songs = 0x30;         // List<GameSong>
    constexpr ptrdiff_t SuggestionCategory_Suggestions = 0x40;
    constexpr ptrdiff_t SuggestionsManager_CurrentSuggestions = 0xC0;
    constexpr ptrdiff_t List_items = 0x10;
    constexpr ptrdiff_t List_size = 0x18;
    constexpr ptrdiff_t Class_vtable_set_CurrentGameSong = 0x1A8;  // GameManager, slot 7: method pointer, MethodInfo
    constexpr ptrdiff_t Class_vtable_GoTo = 0x2E8;                 // GameManager, slot 27

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

    u64 AllButtons() {
        u64 buttons = Buttons<nn::hid::NpadHandheldState>(0x20);
        for (u32 id = 0; id < 8; id++) {
            buttons |= Buttons<nn::hid::NpadFullKeyState>(id) | Buttons<nn::hid::NpadJoyDualState>(id)
                     | Buttons<nn::hid::NpadJoyLeftState>(id) | Buttons<nn::hid::NpadJoyRightState>(id);
        }
        return buttons;
    }

    bool SkipPressed() {
        return (AllButtons() & SKIP_BUTTONS) != 0;
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

namespace {
    /* ---- random song of the current category (ZL + ZR in the song list) ---- */
    constexpr int MAX_SONGS = 4096;
    constexpr u64 RANDOM_COOLDOWN_S = 3;   // the list keeps updating for a moment while the game leaves it

    using BoolGetterFn = bool (*)(void *self, void *method);
    using IntGetterFn = int (*)(void *self, void *method);
    using InitMetadataFn = void (*)(void **slot);
    using WriteBarrierFn = void (*)(void **field, void *value);
    using SetSongFn = void (*)(void *self, void *song, void *method);
    using GoToFn = void (*)(void *self, void *scene_name, bool back, void *method);

    template<typename Fn>
    Fn Game(uintptr_t offset) {
        return reinterpret_cast<Fn>(exl::util::modules::GetTargetOffset(offset));
    }

    u64 Ticks() {
        u64 value;
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(value));
        return value;
    }

    u64 TicksPerSecond() {
        u64 value;
        __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
        return value ? value : 19200000;
    }

    void *s_Songs[MAX_SONGS];      // the songs of the category when the first button went down
    int s_SongCount = 0;
    void *s_Suggestions = nullptr; // BaseSuggestions of that category when it is a suggestion list, else null
    bool s_AnyHeld = true;         // buttons held when the list opens do not count
    bool s_Fired = false;
    u64 s_LastLaunch = 0;
    void *s_LastSong = nullptr;
    u64 s_Random = 0;

    constexpr int SORT_RECOMMENDED = 0, SORT_FAVORITE = 1, SORT_TITLE = 2, SORT_ARTIST = 3;
    constexpr int SEEN_SLOTS = 2 * MAX_SONGS;  // power of two
    void *s_Seen[SEEN_SLOTS];

    /* False when the song was already added (a song can be in two rows). */
    bool FirstTime(void *song) {
        uintptr_t slot = (reinterpret_cast<uintptr_t>(song) >> 4) * 0x9E3779B97F4A7C15ull >> 51;  // 13 bits
        for (int tries = 0; tries < SEEN_SLOTS; tries++, slot = (slot + 1) & (SEEN_SLOTS - 1)) {
            if (s_Seen[slot] == song) {
                return false;
            }
            if (!s_Seen[slot]) {
                s_Seen[slot] = song;
                return true;
            }
        }
        return false;
    }

    /* Adds the songs of a row (GridSongCategory) that can be sung now. */
    void AddRow(void *row) {
        void *category = row ? At<void *>(row, offsets::GridSongCategory_Category) : nullptr;
        void *list = category ? At<void *>(category, offsets::BaseHubCategory_songs) : nullptr;
        void *items = list ? At<void *>(list, offsets::List_items) : nullptr;
        if (!items) {
            return;
        }
        int size = At<int>(list, offsets::List_size);
        uintptr_t capacity = At<uintptr_t>(items, offsets::Array_length);
        auto is_locked = Game<BoolGetterFn>(offsets::GameSong_get_IsLocked);
        auto has_data = Game<BoolGetterFn>(offsets::GameSong_get_CanRetrieveData);
        for (int i = 0; i < size && static_cast<uintptr_t>(i) < capacity && s_SongCount < MAX_SONGS; i++) {
            void *song = At<void *>(items, offsets::Array_items + i * sizeof(void *));
            if (song && !is_locked(song, nullptr) && has_data(song, nullptr) && FirstTime(song)) {
                s_Songs[s_SongCount++] = song;
            }
        }
    }

    constexpr int ROW_NAME_MAX = 96;
    char16_t s_RowName[ROW_NAME_MAX];  // name of the row the last random song was taken from
    int s_RowNameLength = 0;       // 0: none
    int s_RowOrder = -1;           // order of the list then
    char16_t s_SnapshotName[ROW_NAME_MAX];  // the same for the snapshot in use
    int s_SnapshotNameLength = 0;
    int s_SnapshotOrder = -1;

    int RowName(void *row, char16_t *out) {
        void *category = row ? At<void *>(row, offsets::GridSongCategory_Category) : nullptr;
        void *name = category ? At<void *>(category, offsets::BaseHubCategory_Name) : nullptr;
        int length = name ? At<int>(name, offsets::String_length) : 0;
        if (length <= 0 || length > ROW_NAME_MAX) {
            return 0;
        }
        std::memcpy(out, reinterpret_cast<char *>(name) + offsets::String_chars, length * sizeof(char16_t));
        return length;
    }

    /* The row the last random song came from, when the cursor is still on that song in another row. */
    void *RememberedRow(void *grid, void *row, void *rows, int order) {
        void *cover = At<void *>(grid, offsets::SongsGrid_currentCover);
        if (!cover || !rows || !s_LastSong || s_RowNameLength <= 0 || order != s_RowOrder
                || Game<void *(*)(void *, void *)>(offsets::GridSongCover_get_Song)(cover, nullptr) != s_LastSong) {
            return row;
        }
        char16_t name[ROW_NAME_MAX];
        uintptr_t count = At<uintptr_t>(rows, offsets::Array_length);
        for (uintptr_t i = 0; i < count && i < 512; i++) {
            void *other = At<void *>(rows, offsets::Array_items + i * sizeof(void *));
            if (other && RowName(other, name) == s_RowNameLength
                    && std::memcmp(name, s_RowName, s_RowNameLength * sizeof(char16_t)) == 0) {
                return other;
            }
        }
        return row;
    }

    /* The songs a random one is taken from: the row the cursor is in, or every row when the rows are letters. */
    void Snapshot(void *screen) {
        s_SongCount = 0;
        s_Suggestions = nullptr;
        std::memset(s_Seen, 0, sizeof(s_Seen));
        void *grid = At<void *>(screen, offsets::SongsScreen_songsGrid);
        if (!grid || At<u8>(grid, offsets::SongsGrid_state) > 1) {
            return;
        }
        void *playlists = Game<GetInstanceFn>(offsets::PlaylistManager_get_Instance)(nullptr);
        if (playlists && Game<BoolGetterFn>(offsets::PlaylistManager_get_IsInPlaylist)(playlists, nullptr)) {
            return;
        }
        void *row = At<void *>(grid, offsets::SongsGrid_CurrentCategory);
        if (!row) {
            return;
        }
        void *sort = Game<GetInstanceFn>(offsets::SortManager_get_Instance)(nullptr);
        int order = sort ? Game<IntGetterFn>(offsets::SortManager_get_CurrentSortType)(sort, nullptr) : -1;
        void *rows = At<void *>(grid, offsets::SongsGrid_Categories);
        bool whole_list = order == SORT_FAVORITE || order == SORT_TITLE || order == SORT_ARTIST;
        if (!whole_list) {
            row = RememberedRow(grid, row, rows, order);
        }
        void *category = At<void *>(row, offsets::GridSongCategory_Category);
        if (!category) {
            return;
        }
        s_SnapshotOrder = order;
        s_SnapshotNameLength = whole_list ? 0 : RowName(row, s_SnapshotName);
        if (rows && whole_list) {
            uintptr_t count = At<uintptr_t>(rows, offsets::Array_length);
            for (uintptr_t i = 0; i < count && i < 512; i++) {
                AddRow(At<void *>(rows, offsets::Array_items + i * sizeof(void *)));
            }
        }
        if (s_SongCount == 0) {  // the row itself (also if the list of rows gave nothing)
            AddRow(row);
        }
        if (order == SORT_RECOMMENDED && IsClassName(category, "SuggestionCategory")) {
            s_Suggestions = At<void *>(category, offsets::SuggestionCategory_Suggestions);
        }
    }

    /* What the game does when a cover is validated, with a random song of the snapshot. False if it could not. */
    bool Launch() {
        void *manager = Game<GetInstanceFn>(offsets::GameManager_get_Instance)(nullptr);
        void *klass = manager ? At<void *>(manager, offsets::Object_klass) : nullptr;
        auto slot = reinterpret_cast<void **>(exl::util::modules::GetTargetOffset(offsets::String_PlayerSelection4P));
        Game<InitMetadataFn>(offsets::InitializeRuntimeMetadata)(slot);
        void *scene = *slot;
        if (!klass || !scene || (reinterpret_cast<uintptr_t>(scene) & 1) || s_SongCount <= 0) {
            return false;
        }
        s_Random ^= Ticks();
        s_Random ^= s_Random << 13;
        s_Random ^= s_Random >> 7;
        s_Random ^= s_Random << 17;
        int pick = static_cast<int>(s_Random % static_cast<u64>(s_SongCount));
        if (s_SongCount > 1 && s_Songs[pick] == s_LastSong) {  // not the same one twice in a row
            pick = (pick + 1) % s_SongCount;
        }
        void *song = s_Songs[pick];
        s_LastSong = song;
        s_RowOrder = s_SnapshotOrder;
        s_RowNameLength = s_SnapshotNameLength;
        std::memcpy(s_RowName, s_SnapshotName, sizeof(s_RowName));

        void *suggestions = Game<GetInstanceFn>(offsets::SuggestionsManager_get_Instance)(nullptr);
        if (suggestions) {
            auto field = reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(suggestions)
                                                   + offsets::SuggestionsManager_CurrentSuggestions);
            *field = s_Suggestions;
            Game<WriteBarrierFn>(offsets::WriteBarrier)(field, s_Suggestions);
        }
        At<SetSongFn>(klass, offsets::Class_vtable_set_CurrentGameSong)(
            manager, song, At<void *>(klass, offsets::Class_vtable_set_CurrentGameSong + sizeof(void *)));
        At<GoToFn>(klass, offsets::Class_vtable_GoTo)(
            manager, scene, false, At<void *>(klass, offsets::Class_vtable_GoTo + sizeof(void *)));
        return true;
    }

    /* True when a random song was started: the list is not updated that frame (the game returns there too). */
    bool RandomTick(void *screen) {
        u64 buttons = AllButtons();
        bool zl = buttons & BUTTON_ZL, zr = buttons & BUTTON_ZR;
        bool any = zl || zr;
        if (any && !s_AnyHeld) {
            Snapshot(screen);
            s_Fired = false;
        }
        s_AnyHeld = any;
        if (!(zl && zr) || s_Fired) {
            return false;
        }
        s_Fired = true;  // once per press, also when nothing could be started
        u64 now = Ticks();
        if (s_LastLaunch && now - s_LastLaunch < RANDOM_COOLDOWN_S * TicksPerSecond()) {
            return false;
        }
        if (!Launch()) {
            return false;
        }
        s_LastLaunch = now;
        s_SongCount = 0;
        return true;
    }
}

HOOK_DEFINE_TRAMPOLINE(SongsScreenUpdate) {
    static void Callback(void *self, void *method) {
        if (self && RandomTick(self)) {
            return;
        }
        Orig(self, method);
    }
};

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
    SongsScreenUpdate::InstallAtOffset(offsets::SongsScreen_UpdateVx);
}

extern "C" NORETURN void exl_exception_entry() {
    EXL_ABORT("Default exception handler called!");
}
