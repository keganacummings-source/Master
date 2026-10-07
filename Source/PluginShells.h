#pragma once
#include <JuceHeader.h>
#include "Themes.h"
#include "ThemeDecals.h"

// Hardware shells for the Plugin Builder. Slots are normalised inside the face plate.
// The motherboard bay is always occupied first and is the start of the signal chain.
// Each shell also carries its own internal hardware (see HardwareInternals.h) so the
// selected template forms a distinct, special-looking machine.
namespace pb
{
enum class SlotKind { Board, Knob, Fader, Key, Screen, Cosmetic };

struct Slot
{
    SlotKind kind;
    float x, y, w, h;
    const char* name;
};

struct Shell
{
    const char* id;
    const char* name;
    int hiddenFx;
    float hiddenMix;
    const char* silhouette;
    Slot slots[20];
    int slotCount;
    // Custom hardware design (drawn by paintShellBody / paintScreenBezel, in the builder and in Plugin View).
    int shape;       // 0 rounded 1 chamfer 2 pill 3 square 4 notch 5 wedge 6 octagon
    int ears;        // 0 none 1 rack ears 2 side handles 3 feet 4 top handle 5 wings
    int pattern;     // 0 none 1 top band 2 side stripes 3 hatch 4 dots 5 slats 6 grille 7 chevrons
    int screenStyle; // 0 plain 1 brackets 2 CRT 3 notched 4 scanlines 5 round
    float tint;      // hue rotation applied to the case
};

// 30 templates. Every template has exactly one screen (each a different size/position/bezel),
// 6-20 bays and its own case design. Generated and checked for overlaps; bay 0 is always the motherboard AND the shell's screen (the old separate screen bay is retired).
inline const Shell kShells[] = {
    { "console", "Console Deck", 21, 0.10f, "rounded", {
        { SlotKind::Board, 0.040f, 0.080f, 0.460f, 0.340f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.540f, 0.080f, 0.140f, 0.280f, "KNOB A" },
        { SlotKind::Knob, 0.700f, 0.080f, 0.140f, 0.280f, "KNOB B" },
        { SlotKind::Knob, 0.840f, 0.080f, 0.130f, 0.280f, "KNOB C" },
        { SlotKind::Fader, 0.540f, 0.420f, 0.120f, 0.500f, "FADER A" },
        { SlotKind::Fader, 0.680f, 0.420f, 0.120f, 0.500f, "FADER B" },
        { SlotKind::Key, 0.820f, 0.460f, 0.150f, 0.180f, "KEY" },
        { SlotKind::Cosmetic, 0.360f, 0.620f, 0.140f, 0.280f, "VENT" },
        { SlotKind::Cosmetic, 0.820f, 0.700f, 0.150f, 0.220f, "BADGE" }
    }, 10, 0, 1, 1, 2, 0.00f },
    { "tower", "Tower Rack", 48, 0.09f, "square", {
        { SlotKind::Board, 0.100f, 0.280f, 0.800f, 0.160f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.100f, 0.480f, 0.240f, 0.200f, "KNOB A" },
        { SlotKind::Knob, 0.380f, 0.480f, 0.240f, 0.200f, "KNOB B" },
        { SlotKind::Knob, 0.660f, 0.480f, 0.240f, 0.200f, "KNOB C" },
        { SlotKind::Fader, 0.100f, 0.720f, 0.160f, 0.240f, "FADER A" },
        { SlotKind::Fader, 0.300f, 0.720f, 0.160f, 0.240f, "FADER B" },
        { SlotKind::Key, 0.520f, 0.740f, 0.180f, 0.180f, "KEY" },
        { SlotKind::Cosmetic, 0.740f, 0.740f, 0.160f, 0.180f, "RAIL" }
    }, 9, 3, 1, 5, 0, 0.03f },
    { "desk", "Desk Wing", 53, 0.08f, "wedge", {
        { SlotKind::Board, 0.340f, 0.060f, 0.320f, 0.260f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.060f, 0.100f, 0.120f, 0.240f, "KNOB A" },
        { SlotKind::Knob, 0.200f, 0.100f, 0.120f, 0.240f, "KNOB B" },
        { SlotKind::Knob, 0.700f, 0.100f, 0.120f, 0.240f, "KNOB C" },
        { SlotKind::Fader, 0.060f, 0.460f, 0.120f, 0.460f, "FADER A" },
        { SlotKind::Fader, 0.820f, 0.460f, 0.120f, 0.460f, "FADER B" },
        { SlotKind::Key, 0.700f, 0.420f, 0.120f, 0.180f, "KEY" },
        { SlotKind::Cosmetic, 0.200f, 0.620f, 0.140f, 0.260f, "VENT" },
        { SlotKind::Cosmetic, 0.680f, 0.680f, 0.120f, 0.220f, "BADGE" }
    }, 10, 5, 2, 2, 1, -0.04f },
    { "pocket", "Pocket Unit", 15, 0.12f, "pill", {
        { SlotKind::Board, 0.120f, 0.080f, 0.760f, 0.240f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.100f, 0.380f, 0.220f, 0.200f, "KNOB A" },
        { SlotKind::Knob, 0.390f, 0.380f, 0.220f, 0.200f, "KNOB B" },
        { SlotKind::Fader, 0.680f, 0.360f, 0.200f, 0.220f, "FADER" },
        { SlotKind::Key, 0.520f, 0.640f, 0.180f, 0.280f, "KEY" },
        { SlotKind::Cosmetic, 0.740f, 0.640f, 0.180f, 0.280f, "BADGE" }
    }, 7, 2, 3, 4, 4, 0.06f },
    { "slabmixer", "Slab Mixer", 57, 0.13f, "notch", {
        { SlotKind::Board, 0.006f, 0.006f, 0.236f, 0.463f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.254f, 0.006f, 0.128f, 0.247f, "KNOB 1" },
        { SlotKind::Knob, 0.381f, 0.006f, 0.128f, 0.247f, "KNOB 2" },
        { SlotKind::Knob, 0.509f, 0.006f, 0.128f, 0.247f, "KNOB 3" },
        { SlotKind::Knob, 0.254f, 0.253f, 0.128f, 0.247f, "KNOB 4" },
        { SlotKind::Knob, 0.381f, 0.253f, 0.128f, 0.247f, "KNOB 5" },
        { SlotKind::Knob, 0.509f, 0.253f, 0.128f, 0.247f, "KNOB 6" },
        { SlotKind::Knob, 0.254f, 0.500f, 0.128f, 0.247f, "KNOB 7" },
        { SlotKind::Knob, 0.381f, 0.500f, 0.128f, 0.247f, "KNOB 8" },
        { SlotKind::Knob, 0.509f, 0.500f, 0.128f, 0.247f, "KNOB 9" },
        { SlotKind::Knob, 0.254f, 0.747f, 0.128f, 0.247f, "KNOB 10" },
        { SlotKind::Fader, 0.649f, 0.006f, 0.115f, 0.654f, "FADER 1" },
        { SlotKind::Fader, 0.764f, 0.006f, 0.115f, 0.654f, "FADER 2" },
        { SlotKind::Fader, 0.879f, 0.006f, 0.115f, 0.654f, "FADER 3" },
        { SlotKind::Cosmetic, 0.649f, 0.672f, 0.345f, 0.322f, "VENT 1" }
    }, 16, 4, 3, 6, 1, 0.11f },
    { "rackblade", "Rack Blade", 70, 0.11f, "wedge", {
        { SlotKind::Board, 0.006f, 0.436f, 0.336f, 0.326f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.006f, 0.006f, 0.168f, 0.209f, "KEY 1" },
        { SlotKind::Key, 0.174f, 0.006f, 0.168f, 0.209f, "KEY 2" },
        { SlotKind::Key, 0.006f, 0.215f, 0.168f, 0.209f, "KEY 3" },
        { SlotKind::Knob, 0.354f, 0.006f, 0.320f, 0.542f, "KNOB 1" },
        { SlotKind::Knob, 0.674f, 0.006f, 0.320f, 0.542f, "KNOB 2" },
        { SlotKind::Cosmetic, 0.354f, 0.560f, 0.640f, 0.434f, "VENT 1" }
    }, 8, 5, 2, 1, 2, -0.02f },
    { "orbitpad", "Orbit Pad", 83, 0.09f, "octagon", {
        { SlotKind::Board, 0.277f, 0.006f, 0.388f, 0.385f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.006f, 0.129f, 0.206f, "KNOB 1" },
        { SlotKind::Knob, 0.135f, 0.006f, 0.129f, 0.206f, "KNOB 2" },
        { SlotKind::Knob, 0.006f, 0.212f, 0.129f, 0.206f, "KNOB 3" },
        { SlotKind::Knob, 0.135f, 0.212f, 0.129f, 0.206f, "KNOB 4" },
        { SlotKind::Cosmetic, 0.006f, 0.745f, 0.259f, 0.249f, "VENT 1" },
        { SlotKind::Key, 0.277f, 0.403f, 0.194f, 0.296f, "KEY 1" },
        { SlotKind::Key, 0.471f, 0.403f, 0.194f, 0.296f, "KEY 2" },
        { SlotKind::Key, 0.277f, 0.698f, 0.194f, 0.296f, "KEY 3" },
        { SlotKind::Fader, 0.677f, 0.006f, 0.158f, 0.988f, "FADER 1" },
        { SlotKind::Fader, 0.836f, 0.006f, 0.158f, 0.988f, "FADER 2" }
    }, 12, 6, 1, 4, 3, 0.10f },
    { "studiobrick", "Studio Brick", 96, 0.07f, "rounded", {
        { SlotKind::Board, 0.006f, 0.622f, 0.526f, 0.372f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.526f, 0.604f, "VENT 1" },
        { SlotKind::Knob, 0.544f, 0.006f, 0.225f, 0.330f, "KNOB 1" },
        { SlotKind::Knob, 0.769f, 0.006f, 0.225f, 0.330f, "KNOB 2" },
        { SlotKind::Knob, 0.544f, 0.336f, 0.225f, 0.330f, "KNOB 3" }
    }, 6, 0, 0, 7, 4, -0.03f },
    { "stompbox", "Stomp Box", 9, 0.12f, "chamfer", {
        { SlotKind::Board, 0.006f, 0.006f, 0.351f, 0.445f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.463f, 0.351f, 0.531f, "VENT 1" },
        { SlotKind::Key, 0.369f, 0.006f, 0.259f, 0.494f, "KEY 1" },
        { SlotKind::Knob, 0.640f, 0.334f, 0.118f, 0.330f, "KNOB 1" },
        { SlotKind::Knob, 0.758f, 0.334f, 0.118f, 0.330f, "KNOB 2" },
        { SlotKind::Knob, 0.876f, 0.334f, 0.118f, 0.330f, "KNOB 3" },
        { SlotKind::Knob, 0.640f, 0.664f, 0.118f, 0.330f, "KNOB 4" },
        { SlotKind::Knob, 0.758f, 0.664f, 0.118f, 0.330f, "KNOB 5" }
    }, 9, 1, 5, 2, 5, 0.09f },
    { "arcadecab", "Arcade Cab", 22, 0.10f, "pill", {
        { SlotKind::Board, 0.704f, 0.006f, 0.290f, 0.445f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.006f, 0.093f, 0.247f, "KNOB 1" },
        { SlotKind::Knob, 0.099f, 0.006f, 0.093f, 0.247f, "KNOB 2" },
        { SlotKind::Knob, 0.191f, 0.006f, 0.093f, 0.247f, "KNOB 3" },
        { SlotKind::Knob, 0.284f, 0.006f, 0.093f, 0.247f, "KNOB 4" },
        { SlotKind::Knob, 0.006f, 0.253f, 0.093f, 0.247f, "KNOB 5" },
        { SlotKind::Knob, 0.099f, 0.253f, 0.093f, 0.247f, "KNOB 6" },
        { SlotKind::Knob, 0.191f, 0.253f, 0.093f, 0.247f, "KNOB 7" },
        { SlotKind::Knob, 0.284f, 0.253f, 0.093f, 0.247f, "KNOB 8" },
        { SlotKind::Knob, 0.006f, 0.500f, 0.093f, 0.247f, "KNOB 9" },
        { SlotKind::Knob, 0.099f, 0.500f, 0.093f, 0.247f, "KNOB 10" },
        { SlotKind::Knob, 0.191f, 0.500f, 0.093f, 0.247f, "KNOB 11" },
        { SlotKind::Knob, 0.284f, 0.500f, 0.093f, 0.247f, "KNOB 12" },
        { SlotKind::Knob, 0.006f, 0.747f, 0.093f, 0.247f, "KNOB 13" },
        { SlotKind::Knob, 0.099f, 0.747f, 0.093f, 0.247f, "KNOB 14" },
        { SlotKind::Cosmetic, 0.388f, 0.006f, 0.152f, 0.494f, "VENT 1" },
        { SlotKind::Cosmetic, 0.540f, 0.006f, 0.152f, 0.494f, "BADGE 1" },
        { SlotKind::Cosmetic, 0.388f, 0.500f, 0.152f, 0.494f, "RAIL 1" },
        { SlotKind::Cosmetic, 0.540f, 0.500f, 0.152f, 0.494f, "VENT 2" }
    }, 20, 2, 4, 5, 0, -0.04f },
    { "cassettedeck", "Cassette Deck", 35, 0.08f, "square", {
        { SlotKind::Board, 0.469f, 0.668f, 0.525f, 0.326f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Fader, 0.006f, 0.006f, 0.988f, 0.225f, "FADER 1" },
        { SlotKind::Knob, 0.006f, 0.243f, 0.141f, 0.199f, "KNOB 1" },
        { SlotKind::Knob, 0.147f, 0.243f, 0.141f, 0.199f, "KNOB 2" },
        { SlotKind::Knob, 0.288f, 0.243f, 0.141f, 0.199f, "KNOB 3" },
        { SlotKind::Knob, 0.429f, 0.243f, 0.141f, 0.199f, "KNOB 4" },
        { SlotKind::Knob, 0.571f, 0.243f, 0.141f, 0.199f, "KNOB 5" },
        { SlotKind::Knob, 0.712f, 0.243f, 0.141f, 0.199f, "KNOB 6" },
        { SlotKind::Knob, 0.853f, 0.243f, 0.141f, 0.199f, "KNOB 7" },
        { SlotKind::Cosmetic, 0.006f, 0.454f, 0.988f, 0.202f, "VENT 1" }
    }, 11, 3, 3, 0, 1, 0.08f },
    { "radiowave", "Radio Wave", 48, 0.13f, "notch", {
        { SlotKind::Board, 0.595f, 0.365f, 0.399f, 0.629f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.006f, 0.006f, 0.325f, 0.347f, "KEY 1" },
        { SlotKind::Cosmetic, 0.343f, 0.006f, 0.279f, 0.347f, "VENT 1" },
        { SlotKind::Knob, 0.006f, 0.365f, 0.192f, 0.629f, "KNOB 1" },
        { SlotKind::Knob, 0.198f, 0.365f, 0.192f, 0.629f, "KNOB 2" },
        { SlotKind::Knob, 0.391f, 0.365f, 0.192f, 0.629f, "KNOB 3" }
    }, 7, 4, 2, 3, 2, -0.05f },
    { "synthwedge", "Synth Wedge", 61, 0.11f, "wedge", {
        { SlotKind::Board, 0.006f, 0.006f, 0.485f, 0.329f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.347f, 0.110f, 0.371f, "KNOB 1" },
        { SlotKind::Knob, 0.116f, 0.347f, 0.110f, 0.371f, "KNOB 2" },
        { SlotKind::Knob, 0.226f, 0.347f, 0.110f, 0.371f, "KNOB 3" },
        { SlotKind::Knob, 0.335f, 0.347f, 0.110f, 0.371f, "KNOB 4" },
        { SlotKind::Knob, 0.445f, 0.347f, 0.110f, 0.371f, "KNOB 5" },
        { SlotKind::Knob, 0.555f, 0.347f, 0.110f, 0.371f, "KNOB 6" },
        { SlotKind::Knob, 0.665f, 0.347f, 0.110f, 0.371f, "KNOB 7" },
        { SlotKind::Knob, 0.774f, 0.347f, 0.110f, 0.371f, "KNOB 8" },
        { SlotKind::Knob, 0.884f, 0.347f, 0.110f, 0.371f, "KNOB 9" },
        { SlotKind::Cosmetic, 0.006f, 0.730f, 0.298f, 0.264f, "VENT 1" },
        { SlotKind::Fader, 0.316f, 0.730f, 0.339f, 0.264f, "FADER 1" },
        { SlotKind::Fader, 0.655f, 0.730f, 0.339f, 0.264f, "FADER 2" }
    }, 14, 5, 1, 6, 3, 0.07f },
    { "pedalboard", "Pedal Board", 74, 0.09f, "octagon", {
        { SlotKind::Board, 0.006f, 0.552f, 0.469f, 0.442f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.446f, 0.006f, 0.183f, 0.252f, "KEY 1" },
        { SlotKind::Key, 0.629f, 0.006f, 0.183f, 0.252f, "KEY 2" },
        { SlotKind::Key, 0.811f, 0.006f, 0.183f, 0.252f, "KEY 3" },
        { SlotKind::Knob, 0.006f, 0.270f, 0.082f, 0.269f, "KNOB 1" },
        { SlotKind::Knob, 0.088f, 0.270f, 0.082f, 0.269f, "KNOB 2" },
        { SlotKind::Knob, 0.171f, 0.270f, 0.082f, 0.269f, "KNOB 3" },
        { SlotKind::Knob, 0.253f, 0.270f, 0.082f, 0.269f, "KNOB 4" },
        { SlotKind::Knob, 0.335f, 0.270f, 0.082f, 0.269f, "KNOB 5" },
        { SlotKind::Knob, 0.418f, 0.270f, 0.082f, 0.269f, "KNOB 6" },
        { SlotKind::Knob, 0.500f, 0.270f, 0.082f, 0.269f, "KNOB 7" },
        { SlotKind::Knob, 0.582f, 0.270f, 0.082f, 0.269f, "KNOB 8" },
        { SlotKind::Knob, 0.665f, 0.270f, 0.082f, 0.269f, "KNOB 9" },
        { SlotKind::Knob, 0.747f, 0.270f, 0.082f, 0.269f, "KNOB 10" },
        { SlotKind::Knob, 0.829f, 0.270f, 0.082f, 0.269f, "KNOB 11" },
        { SlotKind::Knob, 0.912f, 0.270f, 0.082f, 0.269f, "KNOB 12" },
        { SlotKind::Cosmetic, 0.487f, 0.552f, 0.507f, 0.442f, "VENT 1" }
    }, 18, 6, 0, 1, 4, -0.06f },
    { "flightcase", "Flight Case", 87, 0.07f, "rounded", {
        { SlotKind::Board, 0.263f, 0.548f, 0.212f, 0.446f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.988f, 0.530f, "VENT 1" },
        { SlotKind::Knob, 0.487f, 0.548f, 0.127f, 0.223f, "KNOB 1" },
        { SlotKind::Knob, 0.614f, 0.548f, 0.127f, 0.223f, "KNOB 2" },
        { SlotKind::Knob, 0.741f, 0.548f, 0.127f, 0.223f, "KNOB 3" },
        { SlotKind::Knob, 0.867f, 0.548f, 0.127f, 0.223f, "KNOB 4" },
        { SlotKind::Knob, 0.487f, 0.771f, 0.127f, 0.223f, "KNOB 5" },
        { SlotKind::Knob, 0.614f, 0.771f, 0.127f, 0.223f, "KNOB 6" },
        { SlotKind::Knob, 0.741f, 0.771f, 0.127f, 0.223f, "KNOB 7" }
    }, 10, 0, 5, 4, 5, 0.06f },
    { "labbench", "Lab Bench", 0, 0.12f, "chamfer", {
        { SlotKind::Board, 0.732f, 0.006f, 0.262f, 0.410f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.412f, 0.988f, "VENT 1" },
        { SlotKind::Fader, 0.430f, 0.006f, 0.290f, 0.196f, "FADER 1" },
        { SlotKind::Knob, 0.430f, 0.214f, 0.097f, 0.283f, "KNOB 1" },
        { SlotKind::Knob, 0.527f, 0.214f, 0.097f, 0.283f, "KNOB 2" },
        { SlotKind::Knob, 0.623f, 0.214f, 0.097f, 0.283f, "KNOB 3" },
        { SlotKind::Knob, 0.430f, 0.498f, 0.097f, 0.283f, "KNOB 4" },
        { SlotKind::Knob, 0.527f, 0.498f, 0.097f, 0.283f, "KNOB 5" },
        { SlotKind::Knob, 0.623f, 0.498f, 0.097f, 0.283f, "KNOB 6" },
        { SlotKind::Key, 0.732f, 0.428f, 0.131f, 0.283f, "KEY 1" },
        { SlotKind::Key, 0.863f, 0.428f, 0.131f, 0.283f, "KEY 2" },
        { SlotKind::Key, 0.732f, 0.711f, 0.131f, 0.283f, "KEY 3" }
    }, 13, 1, 4, 7, 0, -0.07f },
    { "satellite", "Satellite", 13, 0.10f, "pill", {
        { SlotKind::Board, 0.006f, 0.551f, 0.414f, 0.443f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.006f, 0.203f, 0.533f, "KNOB 1" },
        { SlotKind::Knob, 0.209f, 0.006f, 0.203f, 0.533f, "KNOB 2" },
        { SlotKind::Knob, 0.413f, 0.006f, 0.203f, 0.533f, "KNOB 3" },
        { SlotKind::Cosmetic, 0.628f, 0.006f, 0.366f, 0.533f, "VENT 1" }
    }, 6, 2, 3, 2, 1, 0.05f },
    { "cockpit", "Cockpit", 26, 0.08f, "square", {
        { SlotKind::Board, 0.006f, 0.464f, 0.489f, 0.530f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.006f, 0.006f, 0.186f, 0.223f, "KEY 1" },
        { SlotKind::Key, 0.006f, 0.229f, 0.186f, 0.223f, "KEY 2" },
        { SlotKind::Cosmetic, 0.204f, 0.006f, 0.212f, 0.223f, "VENT 1" },
        { SlotKind::Cosmetic, 0.204f, 0.229f, 0.212f, 0.223f, "BADGE 1" },
        { SlotKind::Knob, 0.428f, 0.006f, 0.094f, 0.223f, "KNOB 1" },
        { SlotKind::Knob, 0.522f, 0.006f, 0.094f, 0.223f, "KNOB 2" },
        { SlotKind::Knob, 0.617f, 0.006f, 0.094f, 0.223f, "KNOB 3" },
        { SlotKind::Knob, 0.711f, 0.006f, 0.094f, 0.223f, "KNOB 4" },
        { SlotKind::Knob, 0.805f, 0.006f, 0.094f, 0.223f, "KNOB 5" },
        { SlotKind::Knob, 0.900f, 0.006f, 0.094f, 0.223f, "KNOB 6" },
        { SlotKind::Knob, 0.428f, 0.229f, 0.094f, 0.223f, "KNOB 7" },
        { SlotKind::Knob, 0.522f, 0.229f, 0.094f, 0.223f, "KNOB 8" },
        { SlotKind::Knob, 0.617f, 0.229f, 0.094f, 0.223f, "KNOB 9" },
        { SlotKind::Knob, 0.711f, 0.229f, 0.094f, 0.223f, "KNOB 10" },
        { SlotKind::Knob, 0.805f, 0.229f, 0.094f, 0.223f, "KNOB 11" }
    }, 17, 3, 2, 5, 2, -0.08f },
    { "vinyldeck", "Vinyl Deck", 39, 0.13f, "notch", {
        { SlotKind::Board, 0.006f, 0.333f, 0.309f, 0.282f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.530f, 0.006f, 0.464f, 0.315f, "VENT 1" },
        { SlotKind::Knob, 0.327f, 0.333f, 0.111f, 0.282f, "KNOB 1" },
        { SlotKind::Knob, 0.438f, 0.333f, 0.111f, 0.282f, "KNOB 2" },
        { SlotKind::Knob, 0.549f, 0.333f, 0.111f, 0.282f, "KNOB 3" },
        { SlotKind::Knob, 0.660f, 0.333f, 0.111f, 0.282f, "KNOB 4" },
        { SlotKind::Knob, 0.772f, 0.333f, 0.111f, 0.282f, "KNOB 5" },
        { SlotKind::Knob, 0.883f, 0.333f, 0.111f, 0.282f, "KNOB 6" },
        { SlotKind::Key, 0.006f, 0.626f, 0.329f, 0.368f, "KEY 1" },
        { SlotKind::Key, 0.335f, 0.626f, 0.329f, 0.368f, "KEY 2" },
        { SlotKind::Key, 0.665f, 0.626f, 0.329f, 0.368f, "KEY 3" }
    }, 12, 4, 1, 0, 3, 0.04f },
    { "vaultdoor", "Vault Door", 52, 0.11f, "wedge", {
        { SlotKind::Board, 0.006f, 0.548f, 0.302f, 0.446f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.006f, 0.006f, 0.255f, 0.265f, "KEY 1" },
        { SlotKind::Key, 0.261f, 0.006f, 0.255f, 0.265f, "KEY 2" },
        { SlotKind::Key, 0.006f, 0.271f, 0.255f, 0.265f, "KEY 3" },
        { SlotKind::Knob, 0.529f, 0.006f, 0.233f, 0.530f, "KNOB 1" },
        { SlotKind::Knob, 0.761f, 0.006f, 0.233f, 0.530f, "KNOB 2" },
        { SlotKind::Cosmetic, 0.726f, 0.548f, 0.268f, 0.446f, "VENT 1" }
    }, 8, 5, 0, 3, 4, -0.09f },
    { "samplergrid", "Sampler Grid", 65, 0.09f, "octagon", {
        { SlotKind::Board, 0.006f, 0.006f, 0.305f, 0.257f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.323f, 0.006f, 0.224f, 0.257f, "VENT 1" },
        { SlotKind::Cosmetic, 0.547f, 0.006f, 0.224f, 0.257f, "BADGE 1" },
        { SlotKind::Cosmetic, 0.770f, 0.006f, 0.224f, 0.257f, "RAIL 1" },
        { SlotKind::Knob, 0.006f, 0.275f, 0.090f, 0.248f, "KNOB 1" },
        { SlotKind::Knob, 0.096f, 0.275f, 0.090f, 0.248f, "KNOB 2" },
        { SlotKind::Knob, 0.186f, 0.275f, 0.090f, 0.248f, "KNOB 3" },
        { SlotKind::Knob, 0.275f, 0.275f, 0.090f, 0.248f, "KNOB 4" },
        { SlotKind::Knob, 0.365f, 0.275f, 0.090f, 0.248f, "KNOB 5" },
        { SlotKind::Knob, 0.455f, 0.275f, 0.090f, 0.248f, "KNOB 6" },
        { SlotKind::Knob, 0.545f, 0.275f, 0.090f, 0.248f, "KNOB 7" },
        { SlotKind::Knob, 0.635f, 0.275f, 0.090f, 0.248f, "KNOB 8" },
        { SlotKind::Knob, 0.725f, 0.275f, 0.090f, 0.248f, "KNOB 9" },
        { SlotKind::Knob, 0.814f, 0.275f, 0.090f, 0.248f, "KNOB 10" },
        { SlotKind::Knob, 0.904f, 0.275f, 0.090f, 0.248f, "KNOB 11" },
        { SlotKind::Key, 0.006f, 0.535f, 0.172f, 0.459f, "KEY 1" },
        { SlotKind::Key, 0.178f, 0.535f, 0.172f, 0.459f, "KEY 2" },
        { SlotKind::Key, 0.351f, 0.535f, 0.172f, 0.459f, "KEY 3" }
    }, 19, 6, 5, 6, 5, 0.03f },
    { "monoslab", "Mono Slab", 78, 0.07f, "rounded", {
        { SlotKind::Board, 0.006f, 0.422f, 0.343f, 0.572f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.322f, 0.404f, "VENT 1" },
        { SlotKind::Key, 0.646f, 0.006f, 0.348f, 0.404f, "KEY 1" },
        { SlotKind::Knob, 0.361f, 0.422f, 0.211f, 0.572f, "KNOB 1" },
        { SlotKind::Knob, 0.572f, 0.422f, 0.211f, 0.572f, "KNOB 2" },
        { SlotKind::Knob, 0.783f, 0.422f, 0.211f, 0.572f, "KNOB 3" }
    }, 7, 0, 4, 1, 0, -0.10f },
    { "twintower", "Twin Tower", 91, 0.12f, "chamfer", {
        { SlotKind::Board, 0.006f, 0.006f, 0.502f, 0.503f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.521f, 0.135f, 0.236f, "KNOB 1" },
        { SlotKind::Knob, 0.141f, 0.521f, 0.135f, 0.236f, "KNOB 2" },
        { SlotKind::Knob, 0.276f, 0.521f, 0.135f, 0.236f, "KNOB 3" },
        { SlotKind::Knob, 0.411f, 0.521f, 0.135f, 0.236f, "KNOB 4" },
        { SlotKind::Knob, 0.006f, 0.758f, 0.135f, 0.236f, "KNOB 5" },
        { SlotKind::Knob, 0.141f, 0.758f, 0.135f, 0.236f, "KNOB 6" },
        { SlotKind::Knob, 0.276f, 0.758f, 0.135f, 0.236f, "KNOB 7" },
        { SlotKind::Knob, 0.411f, 0.758f, 0.135f, 0.236f, "KNOB 8" },
        { SlotKind::Fader, 0.558f, 0.521f, 0.076f, 0.473f, "FADER 1" },
        { SlotKind::Fader, 0.634f, 0.521f, 0.076f, 0.473f, "FADER 2" },
        { SlotKind::Fader, 0.710f, 0.521f, 0.076f, 0.473f, "FADER 3" },
        { SlotKind::Cosmetic, 0.798f, 0.521f, 0.196f, 0.236f, "VENT 1" },
        { SlotKind::Cosmetic, 0.798f, 0.758f, 0.196f, 0.236f, "BADGE 1" }
    }, 15, 1, 3, 4, 1, 0.02f },
    { "bridgeconsole", "Bridge Console", 4, 0.10f, "pill", {
        { SlotKind::Board, 0.006f, 0.579f, 0.230f, 0.415f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.167f, 0.281f, "VENT 1" },
        { SlotKind::Cosmetic, 0.173f, 0.006f, 0.167f, 0.281f, "BADGE 1" },
        { SlotKind::Cosmetic, 0.006f, 0.287f, 0.167f, 0.281f, "RAIL 1" },
        { SlotKind::Cosmetic, 0.173f, 0.287f, 0.167f, 0.281f, "VENT 2" },
        { SlotKind::Fader, 0.352f, 0.006f, 0.096f, 0.561f, "FADER 1" },
        { SlotKind::Fader, 0.448f, 0.006f, 0.096f, 0.561f, "FADER 2" },
        { SlotKind::Fader, 0.544f, 0.006f, 0.096f, 0.561f, "FADER 3" },
        { SlotKind::Fader, 0.640f, 0.006f, 0.096f, 0.561f, "FADER 4" },
        { SlotKind::Key, 0.749f, 0.006f, 0.245f, 0.281f, "KEY 1" },
        { SlotKind::Key, 0.749f, 0.287f, 0.245f, 0.281f, "KEY 2" },
        { SlotKind::Knob, 0.464f, 0.579f, 0.133f, 0.207f, "KNOB 1" },
        { SlotKind::Knob, 0.596f, 0.579f, 0.133f, 0.207f, "KNOB 2" },
        { SlotKind::Knob, 0.729f, 0.579f, 0.133f, 0.207f, "KNOB 3" },
        { SlotKind::Knob, 0.861f, 0.579f, 0.133f, 0.207f, "KNOB 4" },
        { SlotKind::Knob, 0.464f, 0.787f, 0.133f, 0.207f, "KNOB 5" },
        { SlotKind::Knob, 0.596f, 0.787f, 0.133f, 0.207f, "KNOB 6" },
        { SlotKind::Knob, 0.729f, 0.787f, 0.133f, 0.207f, "KNOB 7" },
        { SlotKind::Knob, 0.861f, 0.787f, 0.133f, 0.207f, "KNOB 8" }
    }, 20, 2, 2, 7, 2, -0.11f },
    { "nanochip", "Nano Chip", 17, 0.08f, "square", {
        { SlotKind::Board, 0.622f, 0.616f, 0.372f, 0.378f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Cosmetic, 0.006f, 0.006f, 0.604f, 0.319f, "VENT 1" },
        { SlotKind::Fader, 0.006f, 0.698f, 0.604f, 0.296f, "FADER 1" },
        { SlotKind::Knob, 0.622f, 0.006f, 0.186f, 0.598f, "KNOB 1" },
        { SlotKind::Knob, 0.808f, 0.006f, 0.186f, 0.598f, "KNOB 2" }
    }, 6, 3, 1, 2, 3, 0.01f },
    { "megamatrix", "Mega Matrix", 30, 0.13f, "notch", {
        { SlotKind::Board, 0.006f, 0.482f, 0.447f, 0.512f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Key, 0.465f, 0.006f, 0.176f, 0.165f, "KEY 1" },
        { SlotKind::Key, 0.641f, 0.006f, 0.176f, 0.165f, "KEY 2" },
        { SlotKind::Key, 0.818f, 0.006f, 0.176f, 0.165f, "KEY 3" },
        { SlotKind::Knob, 0.465f, 0.183f, 0.132f, 0.197f, "KNOB 1" },
        { SlotKind::Knob, 0.597f, 0.183f, 0.132f, 0.197f, "KNOB 2" },
        { SlotKind::Knob, 0.729f, 0.183f, 0.132f, 0.197f, "KNOB 3" },
        { SlotKind::Knob, 0.862f, 0.183f, 0.132f, 0.197f, "KNOB 4" },
        { SlotKind::Knob, 0.465f, 0.380f, 0.132f, 0.197f, "KNOB 5" },
        { SlotKind::Knob, 0.597f, 0.380f, 0.132f, 0.197f, "KNOB 6" },
        { SlotKind::Knob, 0.729f, 0.380f, 0.132f, 0.197f, "KNOB 7" },
        { SlotKind::Fader, 0.465f, 0.589f, 0.265f, 0.124f, "FADER 1" },
        { SlotKind::Fader, 0.729f, 0.589f, 0.265f, 0.124f, "FADER 2" },
        { SlotKind::Cosmetic, 0.465f, 0.725f, 0.132f, 0.269f, "VENT 1" },
        { SlotKind::Cosmetic, 0.597f, 0.725f, 0.132f, 0.269f, "BADGE 1" },
        { SlotKind::Cosmetic, 0.729f, 0.725f, 0.132f, 0.269f, "RAIL 1" },
        { SlotKind::Cosmetic, 0.862f, 0.725f, 0.132f, 0.269f, "VENT 2" }
    }, 18, 4, 0, 5, 4, -0.12f },
    { "ribbondeck", "Ribbon Deck", 43, 0.11f, "wedge", {
        { SlotKind::Board, 0.753f, 0.006f, 0.241f, 0.481f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.006f, 0.177f, 0.240f, "KNOB 1" },
        { SlotKind::Knob, 0.183f, 0.006f, 0.177f, 0.240f, "KNOB 2" },
        { SlotKind::Knob, 0.006f, 0.246f, 0.177f, 0.240f, "KNOB 3" },
        { SlotKind::Key, 0.372f, 0.006f, 0.184f, 0.240f, "KEY 1" },
        { SlotKind::Key, 0.556f, 0.006f, 0.184f, 0.240f, "KEY 2" },
        { SlotKind::Key, 0.372f, 0.246f, 0.184f, 0.240f, "KEY 3" },
        { SlotKind::Cosmetic, 0.546f, 0.499f, 0.448f, 0.495f, "VENT 1" }
    }, 9, 5, 5, 0, 5, 0.00f },
    { "moonbase", "Moon Base", 56, 0.09f, "octagon", {
        { SlotKind::Board, 0.006f, 0.006f, 0.592f, 0.353f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Knob, 0.006f, 0.371f, 0.145f, 0.312f, "KNOB 1" },
        { SlotKind::Knob, 0.151f, 0.371f, 0.145f, 0.312f, "KNOB 2" },
        { SlotKind::Knob, 0.296f, 0.371f, 0.145f, 0.312f, "KNOB 3" },
        { SlotKind::Knob, 0.441f, 0.371f, 0.145f, 0.312f, "KNOB 4" },
        { SlotKind::Knob, 0.585f, 0.371f, 0.145f, 0.312f, "KNOB 5" },
        { SlotKind::Knob, 0.006f, 0.682f, 0.145f, 0.312f, "KNOB 6" },
        { SlotKind::Knob, 0.151f, 0.682f, 0.145f, 0.312f, "KNOB 7" },
        { SlotKind::Knob, 0.296f, 0.682f, 0.145f, 0.312f, "KNOB 8" },
        { SlotKind::Knob, 0.441f, 0.682f, 0.145f, 0.312f, "KNOB 9" },
        { SlotKind::Knob, 0.585f, 0.682f, 0.145f, 0.312f, "KNOB 10" },
        { SlotKind::Cosmetic, 0.742f, 0.371f, 0.252f, 0.312f, "VENT 1" },
        { SlotKind::Cosmetic, 0.742f, 0.682f, 0.252f, 0.312f, "BADGE 1" }
    }, 14, 6, 4, 3, 0, 0.12f },
    { "retrotube", "Retro Tube", 69, 0.07f, "rounded", {
        { SlotKind::Board, 0.483f, 0.006f, 0.242f, 0.377f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Fader, 0.006f, 0.006f, 0.232f, 0.988f, "FADER 1" },
        { SlotKind::Fader, 0.238f, 0.006f, 0.232f, 0.988f, "FADER 2" },
        { SlotKind::Cosmetic, 0.483f, 0.688f, 0.242f, 0.306f, "VENT 1" },
        { SlotKind::Knob, 0.736f, 0.006f, 0.129f, 0.329f, "KNOB 1" },
        { SlotKind::Knob, 0.865f, 0.006f, 0.129f, 0.329f, "KNOB 2" },
        { SlotKind::Knob, 0.736f, 0.335f, 0.129f, 0.329f, "KNOB 3" },
        { SlotKind::Knob, 0.865f, 0.335f, 0.129f, 0.329f, "KNOB 4" },
        { SlotKind::Knob, 0.736f, 0.665f, 0.129f, 0.329f, "KNOB 5" }
    }, 10, 0, 3, 6, 1, -0.01f },
    { "hexcore", "Hex Core", 82, 0.12f, "chamfer", {
        { SlotKind::Board, 0.238f, 0.575f, 0.326f, 0.419f, "SCREEN" },
        { SlotKind::Screen, 0.000f, 0.000f, 0.000f, 0.000f, "-" },
        { SlotKind::Fader, 0.006f, 0.006f, 0.100f, 0.557f, "FADER 1" },
        { SlotKind::Fader, 0.106f, 0.006f, 0.100f, 0.557f, "FADER 2" },
        { SlotKind::Key, 0.218f, 0.006f, 0.154f, 0.278f, "KEY 1" },
        { SlotKind::Key, 0.372f, 0.006f, 0.154f, 0.278f, "KEY 2" },
        { SlotKind::Key, 0.218f, 0.284f, 0.154f, 0.278f, "KEY 3" },
        { SlotKind::Knob, 0.538f, 0.006f, 0.114f, 0.278f, "KNOB 1" },
        { SlotKind::Knob, 0.652f, 0.006f, 0.114f, 0.278f, "KNOB 2" },
        { SlotKind::Knob, 0.766f, 0.006f, 0.114f, 0.278f, "KNOB 3" },
        { SlotKind::Knob, 0.880f, 0.006f, 0.114f, 0.278f, "KNOB 4" },
        { SlotKind::Knob, 0.538f, 0.284f, 0.114f, 0.278f, "KNOB 5" },
        { SlotKind::Knob, 0.652f, 0.284f, 0.114f, 0.278f, "KNOB 6" },
        { SlotKind::Knob, 0.766f, 0.284f, 0.114f, 0.278f, "KNOB 7" },
        { SlotKind::Cosmetic, 0.576f, 0.575f, 0.209f, 0.419f, "VENT 1" },
        { SlotKind::Cosmetic, 0.785f, 0.575f, 0.209f, 0.419f, "BADGE 1" }
    }, 16, 1, 2, 1, 2, 0.11f }
};

inline constexpr int kShellCount = (int) (sizeof(kShells) / sizeof(kShells[0]));

inline juce::String styleToken(int kindId)
{
    switch (kindId)
    {
        case 1: return "dial";
        case 2: return "pointer";
        case 3: return "fader";
        case 4: return "hfader";
        case 5: return "key";
        case 6: return "screen";
        case 7: return "vent";
        case 8: return "badge";
        case 9: return "rail";
        default: return "dial";
    }
}

inline SlotKind styleSlot(const juce::String& style)
{
    if (style == "fader" || style == "hfader" || style == "slider") return SlotKind::Fader;
    if (style == "key") return SlotKind::Key;
    if (style == "screen" || style == "wave") return SlotKind::Screen;
    if (style == "vent" || style == "badge" || style == "rail") return SlotKind::Cosmetic;
    if (style == "board") return SlotKind::Board;
    return SlotKind::Knob;
}

inline bool styleFits(const juce::String& style, SlotKind slot)
{
    // Sound and toggle buttons can sit in any bay except the motherboard.
    if (style == "sound" || style == "button") return slot != SlotKind::Board;
    return styleSlot(style) == slot;
}

inline int cosmeticHiddenFx(const juce::String& style)
{
    if (style == "vent") return 32;
    if (style == "badge") return 14;
    if (style == "rail") return 4;
    return -1;
}

inline juce::Rectangle<float> faceRect(juce::Rectangle<float> panel)
{
    return panel.reduced(16.f, 50.f);
}

inline juce::Rectangle<float> slotRect(juce::Rectangle<float> face, const Slot& slot)
{
    return { face.getX() + slot.x * face.getWidth(), face.getY() + slot.y * face.getHeight(),
             slot.w * face.getWidth(), slot.h * face.getHeight() };
}

inline float shellRadius(const Shell& shell)
{
    static const float r[] = { 16.f, 12.f, 28.f, 8.f, 14.f, 16.f, 12.f };
    return r[juce::jlimit(0, 6, shell.shape)];
}

// The outer case of a template: rounded, chamfered, pill, square, notched, wedge or hex.
inline juce::Path casePath(juce::Rectangle<float> c, const Shell& shell)
{
    juce::Path p;
    const float w = c.getWidth(), h = c.getHeight();
    const float x = c.getX(), y = c.getY(), r = c.getRight(), b = c.getBottom();
    switch (shell.shape)
    {
        case 1:
        {
            const float k = juce::jmin(36.f, h * 0.09f);
            p.startNewSubPath(x + k, y); p.lineTo(r - k, y); p.lineTo(r, y + k); p.lineTo(r, b - k);
            p.lineTo(r - k, b); p.lineTo(x + k, b); p.lineTo(x, b - k); p.lineTo(x, y + k); p.closeSubPath();
            return p.createPathWithRoundedCorners(5.f);
        }
        case 2: p.addRoundedRectangle(c, juce::jmin(h * 0.12f, 34.f)); return p;
        case 3: p.addRoundedRectangle(c, 7.f); return p;
        case 4:
        {
            const float nw = w * 0.16f, nd = juce::jmin(26.f, h * 0.055f), cx = c.getCentreX();
            p.startNewSubPath(x, y); p.lineTo(cx - nw, y); p.lineTo(cx - nw + nd, y + nd); p.lineTo(cx + nw - nd, y + nd);
            p.lineTo(cx + nw, y); p.lineTo(r, y); p.lineTo(r, b); p.lineTo(x, b); p.closeSubPath();
            return p.createPathWithRoundedCorners(6.f);
        }
        case 5:
        {
            const float k = juce::jmin(44.f, h * 0.11f);
            p.startNewSubPath(x + k, y); p.lineTo(r, y); p.lineTo(r, b - k); p.lineTo(r - k, b); p.lineTo(x, b); p.lineTo(x, y + k); p.closeSubPath();
            return p.createPathWithRoundedCorners(5.f);
        }
        case 6:
        {
            const float k = juce::jmin(18.f, w * 0.02f), cy = c.getCentreY();
            p.startNewSubPath(x + k, y); p.lineTo(r - k, y); p.lineTo(r, cy); p.lineTo(r - k, b); p.lineTo(x + k, b); p.lineTo(x, cy); p.closeSubPath();
            return p.createPathWithRoundedCorners(5.f);
        }
        default: p.addRoundedRectangle(c, 16.f); return p;
    }
}

inline void paintShellEars(juce::Graphics& g, juce::Rectangle<float> c, const Shell& shell, const kt::ThemePalette& theme)
{
    const auto fill = kt::c(theme.panel).withRotatedHue(shell.tint * 0.6f).brighter(0.08f);
    const auto edge = kt::c(theme.border);
    const float w = c.getWidth(), h = c.getHeight();
    auto bar = [&](juce::Rectangle<float> r, float rad) {
        g.setColour(fill); g.fillRoundedRectangle(r, rad);
        g.setColour(edge); g.drawRoundedRectangle(r, rad, 1.f);
    };
    switch (shell.ears)
    {
        case 1: // rack ears with screws
            for (bool left : { true, false })
            {
                auto r = juce::Rectangle<float>(left ? c.getX() - 15.f : c.getRight() - 1.f, c.getY() + h * 0.10f, 16.f, h * 0.80f);
                bar(r, 3.f);
                g.setColour(kt::c(theme.muted).withAlpha(0.7f));
                for (float t : { 0.12f, 0.88f }) g.fillEllipse(r.getCentreX() - 3.f, r.getY() + r.getHeight() * t - 3.f, 6.f, 6.f);
            }
            break;
        case 2: // side handles
            for (bool left : { true, false })
                bar({ left ? c.getX() - 13.f : c.getRight() + 3.f, c.getCentreY() - h * 0.16f, 10.f, h * 0.32f }, 5.f);
            break;
        case 3: // feet
            for (float t : { 0.10f, 0.28f, 0.72f, 0.90f })
                bar({ c.getX() + w * t - 14.f, c.getBottom() - 1.f, 28.f, 10.f }, 4.f);
            break;
        case 4: // top handle
            bar({ c.getCentreX() - w * 0.11f, c.getY() - 14.f, w * 0.22f, 15.f }, 7.f);
            break;
        case 5: // wings
            for (bool left : { true, false })
            {
                juce::Path wing;
                const float sx = left ? c.getX() : c.getRight(), dx = left ? -17.f : 17.f;
                wing.startNewSubPath(sx, c.getY() + h * 0.22f); wing.lineTo(sx + dx, c.getCentreY()); wing.lineTo(sx, c.getBottom() - h * 0.22f); wing.closeSubPath();
                g.setColour(fill); g.fillPath(wing); g.setColour(edge); g.strokePath(wing, juce::PathStrokeType(1.f));
            }
            break;
        default: break;
    }
}

// Case body of a template. `ears` is only drawn in Plugin View (the builder panel has no room outside the case).
inline void paintShellBody(juce::Graphics& g, juce::Rectangle<float> c, const Shell& shell, const kt::ThemePalette& theme, bool drawEars)
{
    const auto accent = kt::c(theme.accent).withRotatedHue(shell.tint);
    const auto path = casePath(c, shell);
    if (drawEars) paintShellEars(g, c, shell, theme);
    g.setColour(kt::c(theme.panel).withRotatedHue(shell.tint * 0.6f));
    g.fillPath(path);
    g.setColour(kt::c(theme.border));
    g.strokePath(path, juce::PathStrokeType(1.4f));

    g.saveState();
    g.reduceClipRegion(path);
    const juce::Rectangle<float> band(c.getX() + 22.f, c.getBottom() - 44.f, juce::jmax(10.f, c.getWidth() - 44.f), 32.f);
    g.setColour(accent.withAlpha(0.30f));
    switch (shell.pattern)
    {
        case 1: g.fillRect(c.getX() + 30.f, c.getY() + 5.f, c.getWidth() - 60.f, 4.f); break;
        case 2:
            for (float o : { 5.f, 10.f })
            {
                g.fillRect(c.getX() + o, c.getY() + 40.f, 2.f, c.getHeight() - 80.f);
                g.fillRect(c.getRight() - o - 2.f, c.getY() + 40.f, 2.f, c.getHeight() - 80.f);
            }
            break;
        case 3:
            g.saveState(); g.reduceClipRegion(band.toNearestInt());
            for (float x = band.getX() - band.getHeight(); x < band.getRight(); x += 10.f)
                g.drawLine(x, band.getBottom(), x + band.getHeight(), band.getY(), 1.2f);
            g.restoreState();
            break;
        case 4:
            for (float x = band.getX() + 4.f; x < band.getRight(); x += 14.f)
                for (float y = band.getY() + 4.f; y < band.getBottom(); y += 10.f)
                    g.fillEllipse(x - 1.5f, y - 1.5f, 3.f, 3.f);
            break;
        case 5:
            for (int i = 0; i < 4; ++i)
                g.fillRoundedRectangle(band.getCentreX() - band.getWidth() * 0.22f, band.getY() + 3.f + (float) i * 7.f, band.getWidth() * 0.44f, 3.f, 1.5f);
            break;
        case 6:
            for (float x = band.getCentreX() - band.getWidth() * 0.25f; x < band.getCentreX() + band.getWidth() * 0.25f; x += 7.f)
                g.fillRoundedRectangle(x, band.getY() + 2.f, 3.f, band.getHeight() - 4.f, 1.5f);
            break;
        case 7:
            for (float x = band.getX() + 6.f; x < band.getRight() - 12.f; x += 24.f)
            {
                juce::Path chev;
                chev.startNewSubPath(x, band.getY() + 4.f); chev.lineTo(x + 10.f, band.getCentreY()); chev.lineTo(x, band.getBottom() - 4.f);
                g.strokePath(chev, juce::PathStrokeType(2.f));
            }
            break;
        default: break;
    }
    paintThemeDecals(g, c, theme, (int) (&shell - kShells));
    g.restoreState();
}


// ---- Screens: every motherboard IS the plugin's screen. Right-click it -> Change Screen. -----------------
inline constexpr const char* kScreenTypes[] = { "Plain Glass", "Corner Brackets", "CRT Tube", "Notched Panel", "Scanline Monitor", "Round Porthole" };
inline constexpr int kScreenTypeCount = 6;

inline int boardScreenTypeOf(const juce::ValueTree& uiState, int fallback)
{
    for (int i = 0; i < uiState.getNumChildren(); ++i)
    {
        auto child = uiState.getChild(i);
        if (child.hasType("w") && child.getProperty("kind").toString() == "board")
            return juce::jlimit(0, kScreenTypeCount - 1, (int) child.getProperty("screenType", fallback));
    }
    return juce::jlimit(0, kScreenTypeCount - 1, fallback);
}

// Paints the live screen face (scope + type specific glass). Used by the builder widget and by Plugin View.
inline void paintScreenFace(juce::Graphics& g, juce::Rectangle<float> r, int type, const kt::ThemePalette& theme,
                            const float* scope, int n, const juce::String& caption)
{
    type = juce::jlimit(0, kScreenTypeCount - 1, type);
    const auto accent = kt::c(theme.accent);
    const float m = juce::jmin(r.getWidth(), r.getHeight());
    juce::Path clip;
    if (type == 5) clip.addRoundedRectangle(r, m * 0.5f); else clip.addRoundedRectangle(r, type == 2 ? m * 0.20f : 5.f);
    g.saveState();
    g.reduceClipRegion(clip);
    g.setColour(kt::c(theme.bg).darker(0.55f));
    g.fillRect(r);
    g.setColour(accent.withAlpha(0.07f));
    for (float x = r.getX() + r.getWidth() / 8.f; x < r.getRight(); x += r.getWidth() / 8.f) g.drawVerticalLine((int) x, r.getY(), r.getBottom());
    for (float y = r.getY() + r.getHeight() / 4.f; y < r.getBottom(); y += r.getHeight() / 4.f) g.drawHorizontalLine((int) y, r.getX(), r.getRight());
    if (scope != nullptr && n > 1)
    {
        juce::Path wave;
        const float mid = r.getCentreY();
        for (int i = 0; i < n; ++i)
        {
            const float x = r.getX() + 4.f + (r.getWidth() - 8.f) * (float) i / (float) (n - 1);
            const float y = mid - juce::jlimit(-1.f, 1.f, scope[i]) * r.getHeight() * 0.40f;
            if (i == 0) wave.startNewSubPath(x, y); else wave.lineTo(x, y);
        }
        g.setColour(accent.withAlpha(0.22f));
        g.strokePath(wave, juce::PathStrokeType(4.f));
        g.setColour(accent);
        g.strokePath(wave, juce::PathStrokeType(1.6f));
    }
    if (type == 2)
    {
        juce::ColourGradient vg(juce::Colours::transparentBlack, r.getCentre(), juce::Colours::black.withAlpha(0.55f), r.getTopLeft(), true);
        g.setGradientFill(vg);
        g.fillRect(r);
    }
    if (type == 4)
    {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        for (float y = r.getY(); y < r.getBottom(); y += 3.f) g.drawHorizontalLine((int) y, r.getX(), r.getRight());
    }
    g.restoreState();
    if (caption.isNotEmpty() && m > 44.f)
    {
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 9.5f, true));
        g.drawText(caption, r.reduced(type == 5 ? m * 0.18f : 8.f, 4.f).removeFromTop(13.f), juce::Justification::centredLeft, true);
    }
}

// The one screen of every template gets its own bezel so no two screens look alike.
inline void paintScreenBezel(juce::Graphics& g, juce::Rectangle<float> r, int style, const kt::ThemePalette& theme)
{
    const auto accent = kt::c(theme.accent);
    const float m = juce::jmin(r.getWidth(), r.getHeight());
    switch (style)
    {
        case 1: // corner brackets
        {
            const float len = juce::jmin(18.f, m * 0.3f);
            g.setColour(accent.withAlpha(0.85f));
            for (int i = 0; i < 4; ++i)
            {
                const bool right = (i & 1) != 0, bottom = (i & 2) != 0;
                const float cx = right ? r.getRight() : r.getX(), cy = bottom ? r.getBottom() : r.getY();
                const float dx = right ? -len : len, dy = bottom ? -len : len;
                g.drawLine(cx, cy, cx + dx, cy, 2.6f);
                g.drawLine(cx, cy, cx, cy + dy, 2.6f);
            }
            break;
        }
        case 2: // CRT
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillRoundedRectangle(r, m * 0.22f);
            g.setColour(accent.withAlpha(0.18f));
            g.drawRoundedRectangle(r.reduced(3.f), m * 0.20f, 5.f);
            g.setColour(accent.withAlpha(0.85f));
            g.drawRoundedRectangle(r, m * 0.22f, 2.f);
            break;
        case 3: // notched corner
        {
            const float k = juce::jmin(20.f, m * 0.3f);
            juce::Path p;
            p.startNewSubPath(r.getX(), r.getY()); p.lineTo(r.getRight() - k, r.getY()); p.lineTo(r.getRight(), r.getY() + k);
            p.lineTo(r.getRight(), r.getBottom()); p.lineTo(r.getX() + k, r.getBottom()); p.lineTo(r.getX(), r.getBottom() - k); p.closeSubPath();
            g.setColour(juce::Colours::black.withAlpha(0.25f)); g.fillPath(p);
            g.setColour(accent.withAlpha(0.9f)); g.strokePath(p, juce::PathStrokeType(2.f));
            break;
        }
        case 4: // scanlines
            g.setColour(juce::Colours::black.withAlpha(0.30f));
            g.fillRoundedRectangle(r, 5.f);
            g.setColour(accent.withAlpha(0.14f));
            for (float y = r.getY() + 3.f; y < r.getBottom() - 2.f; y += 4.f) g.drawLine(r.getX() + 3.f, y, r.getRight() - 3.f, y, 1.f);
            g.setColour(accent.withAlpha(0.7f));
            g.drawRoundedRectangle(r, 5.f, 1.6f);
            break;
        case 5: // round bezel
            g.setColour(accent.withAlpha(0.8f));
            g.drawRoundedRectangle(r, m * 0.5f, 2.f);
            g.setColour(accent.withAlpha(0.3f));
            g.drawRoundedRectangle(r.reduced(5.f), m * 0.5f - 5.f, 1.f);
            break;
        default: // plain double border
            g.setColour(accent.withAlpha(0.75f));
            g.drawRoundedRectangle(r, 6.f, 1.8f);
            g.setColour(accent.withAlpha(0.25f));
            g.drawRoundedRectangle(r.reduced(4.f), 4.f, 1.f);
            break;
    }
}

class BuilderCanvas : public juce::Component
{
public:
    int shellIndex = 0;
    bool placing = false;
    juce::String armedStyle;
    kt::ThemePalette theme = kt::kThemes[0];
    std::function<void(int)> onSlot;
    std::function<void(int, juce::Point<int>)> onRightClick;
    std::function<bool(int)> occupied;
    std::function<juce::Point<float>(int)> anchor;
    std::function<int(int)> parentOf;       // bay a part is connected into (0 = motherboard)
    std::function<void()> onBackgroundClick; // plain click on empty case = deselect
    int selectedSlot = -1;
    int hoverSlot = -1;
    int screenType = -1; // chosen screen for the motherboard (-1 = template default)

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        const auto accent = kt::c(theme.accent);

        g.setColour(kt::c(theme.panel).withAlpha(0.96f));
        g.fillRoundedRectangle(bounds, 14.f);
        g.setColour(kt::c(theme.border));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 14.f, 1.f);
        paintShellBody(g, bounds, shell, theme, false);

        // Clean header: template name on one line, chain hint below it.
        g.setColour(accent);
        g.setFont(kt::font(theme, 15.f, true));
        g.drawText("PLUGIN BUILDER", 18, 12, 220, 18, juce::Justification::left);
        g.setColour(kt::c(theme.muted));
        g.setFont(kt::font(theme, 11.5f));
        g.drawText(juce::String(shell.name) + "  -  click a part, then right-click a free bay: the new part plugs into it", 18, 31, getWidth() - 36, 15, juce::Justification::left);

        auto face = faceRect(bounds);
        const float radius = shellRadius(shell);

        // Faceplate with a per-template tint so each hardware reads special.
        g.setColour(kt::c(theme.bg).withAlpha(0.88f));
        g.fillRoundedRectangle(face, radius);
        g.setColour(accent.withAlpha(0.45f));
        g.drawRoundedRectangle(face, radius, 1.4f);

        // Shell trim: corner screws, LED strip and a name badge - like real hardware.
        g.setColour(kt::c(theme.muted).withAlpha(0.65f));
        for (auto p : { juce::Point<float>(face.getX() + 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getY() + 10.f),
                        juce::Point<float>(face.getX() + 10.f, face.getBottom() - 10.f),
                        juce::Point<float>(face.getRight() - 10.f, face.getBottom() - 10.f) })
        {
            g.fillEllipse(p.x - 2.5f, p.y - 2.5f, 5.f, 5.f);
        }
        for (int i = 0; i < 3; ++i)
        {
            g.setColour(accent.withAlpha(i == 0 ? 0.9f : 0.3f));
            g.fillRect(face.getRight() - 34.f + (float) i * 9.f, face.getY() + 8.f, 5.f, 5.f);
        }
        g.setColour(accent);
        g.setFont(kt::font(theme, 9.f, true));
        g.drawText(juce::String(shell.name).toUpperCase(), (int) face.getRight() - 130, (int) face.getBottom() - 22, 120, 13, juce::Justification::centredRight);
        g.setColour(accent.withAlpha(0.5f));
        g.drawLine(face.getRight() - 130, face.getBottom() - 9.f, face.getRight() - 20.f, face.getBottom() - 9.f, 1.2f);

        // Bay wiring: every filled bay is joined back to the motherboard, lego-style.
        for (int i = 1; i < shell.slotCount; ++i)
        {
            if (occupied && occupied(i) && anchor)
            {
                int par = parentOf ? parentOf(i) : 0;
                if (par < 0 || par == i || par >= shell.slotCount) par = 0;
                auto a = anchor(par);
                auto b = anchor(i);
                if (a.x > 1.f && b.x > 1.f)
                {
                    juce::Path wire;
                    wire.startNewSubPath(a);
                    wire.cubicTo(a.x, (a.y + b.y) * 0.5f, b.x, (a.y + b.y) * 0.5f, b.x, b.y);
                    const bool hotWire = (selectedSlot >= 0 && (par == selectedSlot || i == selectedSlot));
                    g.setColour(hotWire ? kt::c(theme.pegHot) : accent.withAlpha(0.85f));
                    g.strokePath(wire, juce::PathStrokeType(hotWire ? 3.2f : 2.0f));
                    g.setColour(kt::c(theme.pegHot));
                    g.fillEllipse(b.x - 3.f, b.y - 3.f, 6.f, 6.f);
                }
            }
        }

        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f || slot.h < 0.02f) continue;
            auto r = slotRect(face, slot).reduced(3.f);
            const bool taken = occupied && occupied(i);
            const bool fits = placing && ! taken && styleFits(armedStyle, slot.kind);
            const bool hovered = placing && i == hoverSlot && fits;

            // Bay plate.
            g.setColour(fits ? accent.withAlpha(hovered ? 0.38f : 0.26f) : kt::c(theme.panel).withAlpha(taken ? 0.05f : 0.42f));
            g.fillRoundedRectangle(r, 8.f);
            g.setColour(fits ? accent : kt::c(theme.border).withAlpha(taken ? 0.25f : 0.7f));
            g.drawRoundedRectangle(r, 8.f, fits ? (hovered ? 2.4f : 1.8f) : 1.f);
            if (slot.kind == SlotKind::Board) paintScreenBezel(g, r, screenType >= 0 ? screenType : shell.screenStyle, theme);

            // Lego studs: pegs at the four corners click parts into the bay.
            const float pegR = juce::jmin(3.4f, r.getWidth() * 0.12f);
            const float inset = juce::jmax(7.f, juce::jmin(r.getWidth(), r.getHeight()) * 0.14f);
            for (auto c : { juce::Point<float>(r.getX() + inset, r.getY() + inset),
                            juce::Point<float>(r.getRight() - inset, r.getY() + inset),
                            juce::Point<float>(r.getX() + inset, r.getBottom() - inset),
                            juce::Point<float>(r.getRight() - inset, r.getBottom() - inset) })
            {
                if (taken)
                {
                    g.setColour(accent.withAlpha(0.35f));
                    g.fillEllipse(c.x - pegR * 0.7f, c.y - pegR * 0.7f, pegR * 1.4f, pegR * 1.4f);
                }
                else
                {
                    g.setColour(fits ? kt::c(theme.pegHot) : kt::c(theme.peg).withAlpha(0.85f));
                    g.fillEllipse(c.x - pegR, c.y - pegR, pegR * 2.f, pegR * 2.f);
                    if (fits)
                    {
                        g.setColour(kt::c(theme.pegHot).withAlpha(0.35f));
                        g.drawEllipse(c.x - pegR - 2.f, c.y - pegR - 2.f, pegR * 2.f + 4.f, pegR * 2.f + 4.f, 1.f);
                    }
                }
            }

            if (! taken)
            {
                g.setColour(fits ? kt::c(theme.text) : kt::c(theme.muted));
                g.setFont(kt::font(theme, 10.5f, true));
                g.drawFittedText(slot.name, r.reduced(6.f).toNearestInt(), juce::Justification::centred, 2);
            }
        }
    }

    int slotAt(juce::Point<float> pos) const
    {
        if (! placing) return -1;
        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f) continue;
            if (occupied && occupied(i)) continue;
            if (! styleFits(armedStyle, slot.kind)) continue;
            if (slotRect(face, slot).contains(pos)) return i;
        }
        return -1;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int hit = slotAt(e.position);
        if (hit != hoverSlot) { hoverSlot = hit; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoverSlot != -1) { hoverSlot = -1; repaint(); }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
            auto face = faceRect(getLocalBounds().toFloat());
            int hit = -1;
            for (int i = 0; i < shell.slotCount; ++i)
                if (slotRect(face, shell.slots[i]).contains(e.position)) { hit = i; break; }
            if (onRightClick) onRightClick(hit, e.getScreenPosition());
            return;
        }
        if (! placing || ! onSlot)
        {
            if (onBackgroundClick) onBackgroundClick();
            return;
        }
        const auto& shell = kShells[juce::jlimit(0, kShellCount - 1, shellIndex)];
        auto face = faceRect(getLocalBounds().toFloat());
        for (int i = 0; i < shell.slotCount; ++i)
        {
            const auto& slot = shell.slots[i];
            if (slot.w < 0.02f) continue;
            if (occupied && occupied(i)) continue;
            if (! styleFits(armedStyle, slot.kind)) continue;
            if (slotRect(face, slot).contains(e.position))
            {
                onSlot(i);
                return;
            }
        }
    }
};
}