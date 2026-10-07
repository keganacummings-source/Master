#pragma once
#include <JuceHeader.h>
#include "Themes.h"

// Internal hardware that lives inside each hardware shell. Indexes line up with pb::kShells,
// so every selected template carries its own special parts: the board that hosts the chain,
// the power cell, cap banks, cooling, cabling and expansion room that let good pluggins be built.
// The same data drives the builder preview and the Geek x-ray inside Plugin View.
namespace hb
{
enum class PartKind { Board, Battery, Cap, Ribbon, Fan, Psu, Ram, Port, Antenna, Header, Drive };

struct HardwarePart
{
    const char* name;
    PartKind kind;
    float x, y, w, h;      // normalized to the case interior
    const char* blurb;     // what the piece does for the machine
    bool live = false;     // true = the Geek tooltip shows live settings
};

struct ShellInternals
{
    HardwarePart parts[7];
    int count;
};

inline constexpr ShellInternals kInternals[30] = {
    { { // Console Deck - a proper studio workhorse
        { "MAINBOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain. Every bay is wired from here.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the machine alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the power rails for clean headroom.", false },
        { "BUS RIBBON", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio between the bay and the board.", false },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Spins faster when the chain gets hot.", false },
        { "I/O PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where your sound enters and leaves the box.", false },
        { "EXPANSION HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for future bays.", false }
    }, 7 },
    { { // Tower Rack - stacked server muscle
        { "TOWER BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Runs the whole rack. All bays report to it.", true },
        { "RAM STICKS", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the live previews.", false },
        { "PSU MODULE", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every slot a steady rail.", true },
        { "DRIVE BAY", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores your saved machines.", false },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Holds charge for the big transients.", false },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Moves the heat out the back.", false },
        { "CABLE BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Runs the signal down the rack spine.", false }
    }, 7 },
    { { // Desk Wing - broad and battery-friendly
        { "WING BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "The heart of the desk. Hosts the chain.", true },
        { "BATTERY BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Long-life bank for untethered jams.", true },
        { "CAP ROW", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Keeps the faders fed under load.", false },
        { "DESK FAN", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Whisper cooling for the wing.", false },
        { "PATCH PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Patch your signal in and out.", false },
        { "BUS RIBBON", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Ties the board to the patch bay.", false },
        { "EXPANSION HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Room to grow.", false }
    }, 7 },
    { { // Pocket Unit - tiny but mighty
        { "MICRO BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "The little board that runs the show.", true },
        { "LI-ION CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "One cell, many jams.", true },
        { "MICRO CAPS", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Tiny caps for tiny spikes.", false },
        { "ANTENNA", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share, always ready.", false },
        { "CHARGE PORT", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Top it up before the gig.", false },
        { "COOLING VENT", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Passive cooling slot.", false },
        { "SPARE HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Two pins left for you.", false }
    }, 7 },
    { { // Slab Mixer
        { "SLAB BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Slab Mixer.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Slab Mixer alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Slab Mixer power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Slab Mixer." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Slab Mixer cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Slab Mixer." },
        { "PIN HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Slab Mixer." }
    }, 7 },
    { { // Rack Blade
        { "RACK BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Rack Blade.", true },
        { "RAM STICKS", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Rack Blade." },
        { "POWER BRICK", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Rack Blade.", true },
        { "SAMPLE STORE", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Rack Blade." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Rack Blade power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Rack Blade cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Rack Blade." }
    }, 7 },
    { { // Orbit Pad
        { "ORBIT BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Orbit Pad.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Orbit Pad alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Orbit Pad power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Orbit Pad cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Orbit Pad." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Orbit Pad." },
        { "EXPANSION HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Orbit Pad." }
    }, 7 },
    { { // Studio Brick
        { "STUDIO BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Studio Brick.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Studio Brick alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Studio Brick power rails." },
        { "AERIAL", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Studio Brick." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Studio Brick." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Studio Brick cool under load." },
        { "PIN HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Studio Brick." }
    }, 7 },
    { { // Stomp Box
        { "STOMP BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Stomp Box.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Stomp Box alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Stomp Box power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Stomp Box." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Stomp Box cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Stomp Box." },
        { "AUX HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Stomp Box." }
    }, 7 },
    { { // Arcade Cab
        { "ARCADE BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Arcade Cab.", true },
        { "BUFFER RAM", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Arcade Cab." },
        { "RAIL SUPPLY", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Arcade Cab.", true },
        { "DRIVE BAY", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Arcade Cab." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Arcade Cab power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Arcade Cab cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Arcade Cab." }
    }, 7 },
    { { // Cassette Deck
        { "CASSETTE BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Cassette Deck.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Cassette Deck alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Cassette Deck power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Cassette Deck cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Cassette Deck." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Cassette Deck." },
        { "PIN HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Cassette Deck." }
    }, 7 },
    { { // Radio Wave
        { "RADIO BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Radio Wave.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Radio Wave alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Radio Wave power rails." },
        { "WIRELESS COIL", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Radio Wave." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Radio Wave." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Radio Wave cool under load." },
        { "AUX HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Radio Wave." }
    }, 7 },
    { { // Synth Wedge
        { "SYNTH BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Synth Wedge.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Synth Wedge alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Synth Wedge power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Synth Wedge." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Synth Wedge cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Synth Wedge." },
        { "EXPANSION HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Synth Wedge." }
    }, 7 },
    { { // Pedal Board
        { "PEDAL BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Pedal Board.", true },
        { "CACHE MODULE", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Pedal Board." },
        { "PSU MODULE", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Pedal Board.", true },
        { "MEMORY DRIVE", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Pedal Board." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Pedal Board power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Pedal Board cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Pedal Board." }
    }, 7 },
    { { // Flight Case
        { "FLIGHT BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Flight Case.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Flight Case alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Flight Case power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Flight Case cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Flight Case." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Flight Case." },
        { "AUX HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Flight Case." }
    }, 7 },
    { { // Lab Bench
        { "LAB BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Lab Bench.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Lab Bench alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Lab Bench power rails." },
        { "ANTENNA", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Lab Bench." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Lab Bench." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Lab Bench cool under load." },
        { "EXPANSION HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Lab Bench." }
    }, 7 },
    { { // Satellite
        { "SATELLITE BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Satellite.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Satellite alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Satellite power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Satellite." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Satellite cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Satellite." },
        { "PIN HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Satellite." }
    }, 7 },
    { { // Cockpit
        { "COCKPIT BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Cockpit.", true },
        { "RAM STICKS", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Cockpit." },
        { "POWER BRICK", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Cockpit.", true },
        { "SAMPLE STORE", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Cockpit." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Cockpit power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Cockpit cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Cockpit." }
    }, 7 },
    { { // Vinyl Deck
        { "VINYL BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Vinyl Deck.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Vinyl Deck alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Vinyl Deck power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Vinyl Deck cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Vinyl Deck." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Vinyl Deck." },
        { "EXPANSION HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Vinyl Deck." }
    }, 7 },
    { { // Vault Door
        { "VAULT BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Vault Door.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Vault Door alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Vault Door power rails." },
        { "AERIAL", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Vault Door." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Vault Door." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Vault Door cool under load." },
        { "PIN HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Vault Door." }
    }, 7 },
    { { // Sampler Grid
        { "SAMPLER BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Sampler Grid.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Sampler Grid alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Sampler Grid power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Sampler Grid." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Sampler Grid cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Sampler Grid." },
        { "AUX HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Sampler Grid." }
    }, 7 },
    { { // Mono Slab
        { "MONO BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Mono Slab.", true },
        { "BUFFER RAM", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Mono Slab." },
        { "RAIL SUPPLY", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Mono Slab.", true },
        { "DRIVE BAY", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Mono Slab." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Mono Slab power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Mono Slab cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Mono Slab." }
    }, 7 },
    { { // Twin Tower
        { "TWIN BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Twin Tower.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Twin Tower alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Twin Tower power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Twin Tower cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Twin Tower." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Twin Tower." },
        { "PIN HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Twin Tower." }
    }, 7 },
    { { // Bridge Console
        { "BRIDGE BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Bridge Console.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Bridge Console alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Bridge Console power rails." },
        { "WIRELESS COIL", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Bridge Console." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Bridge Console." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Bridge Console cool under load." },
        { "AUX HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Bridge Console." }
    }, 7 },
    { { // Nano Chip
        { "NANO BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Nano Chip.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Nano Chip alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Nano Chip power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Nano Chip." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Nano Chip cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Nano Chip." },
        { "EXPANSION HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Nano Chip." }
    }, 7 },
    { { // Mega Matrix
        { "MEGA BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Mega Matrix.", true },
        { "CACHE MODULE", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Mega Matrix." },
        { "PSU MODULE", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Mega Matrix.", true },
        { "MEMORY DRIVE", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Mega Matrix." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Mega Matrix power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Mega Matrix cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Mega Matrix." }
    }, 7 },
    { { // Ribbon Deck
        { "RIBBON BOARD", PartKind::Board, 0.05f, 0.08f, 0.44f, 0.34f, "Hosts the FX chain for the Ribbon Deck.", true },
        { "CELL BANK", PartKind::Battery, 0.05f, 0.52f, 0.34f, 0.36f, "Keeps the Ribbon Deck alive between sessions.", true },
        { "CAP BANK", PartKind::Cap, 0.44f, 0.60f, 0.16f, 0.26f, "Smooths the Ribbon Deck power rails." },
        { "TURBINE", PartKind::Fan, 0.66f, 0.54f, 0.22f, 0.30f, "Keeps the Ribbon Deck cool under load." },
        { "LINK PORT", PartKind::Port, 0.66f, 0.10f, 0.28f, 0.30f, "Where sound enters and leaves the Ribbon Deck." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.42f, 0.10f, 0.20f, 0.34f, "Carries audio around the Ribbon Deck." },
        { "AUX HEADER", PartKind::Header, 0.90f, 0.58f, 0.06f, 0.16f, "Free pins for growing the Ribbon Deck." }
    }, 7 },
    { { // Moon Base
        { "MOON BOARD", PartKind::Board, 0.06f, 0.06f, 0.66f, 0.40f, "Hosts the FX chain for the Moon Base.", true },
        { "POWER CELL", PartKind::Battery, 0.06f, 0.56f, 0.40f, 0.36f, "Keeps the Moon Base alive between sessions.", true },
        { "CAP ROW", PartKind::Cap, 0.54f, 0.62f, 0.18f, 0.22f, "Smooths the Moon Base power rails." },
        { "ANTENNA", PartKind::Antenna, 0.80f, 0.06f, 0.14f, 0.36f, "Wireless share for the Moon Base." },
        { "JACK BLOCK", PartKind::Port, 0.76f, 0.62f, 0.18f, 0.20f, "Where sound enters and leaves the Moon Base." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.06f, 0.14f, 0.18f, "Keeps the Moon Base cool under load." },
        { "EXPANSION HEADER", PartKind::Header, 0.62f, 0.34f, 0.16f, 0.14f, "Free pins for growing the Moon Base." }
    }, 7 },
    { { // Retro Tube
        { "RETRO BOARD", PartKind::Board, 0.06f, 0.07f, 0.50f, 0.44f, "Hosts the FX chain for the Retro Tube.", true },
        { "BATTERY", PartKind::Battery, 0.06f, 0.58f, 0.26f, 0.34f, "Keeps the Retro Tube alive between sessions.", true },
        { "MICRO CAPS", PartKind::Cap, 0.36f, 0.62f, 0.18f, 0.24f, "Smooths the Retro Tube power rails." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.58f, 0.08f, 0.34f, 0.30f, "Carries audio around the Retro Tube." },
        { "COOLING FAN", PartKind::Fan, 0.62f, 0.50f, 0.24f, 0.32f, "Keeps the Retro Tube cool under load." },
        { "PATCH PORT", PartKind::Port, 0.88f, 0.10f, 0.08f, 0.26f, "Where sound enters and leaves the Retro Tube." },
        { "PIN HEADER", PartKind::Header, 0.84f, 0.58f, 0.12f, 0.18f, "Free pins for growing the Retro Tube." }
    }, 7 },
    { { // Hex Core
        { "HEX BOARD", PartKind::Board, 0.06f, 0.06f, 0.88f, 0.28f, "Hosts the FX chain for the Hex Core.", true },
        { "RAM STICKS", PartKind::Ram, 0.08f, 0.40f, 0.44f, 0.18f, "Scratch memory for the Hex Core." },
        { "POWER BRICK", PartKind::Psu, 0.60f, 0.40f, 0.34f, 0.18f, "Feeds every bay of the Hex Core.", true },
        { "SAMPLE STORE", PartKind::Drive, 0.08f, 0.66f, 0.36f, 0.26f, "Stores presets inside the Hex Core." },
        { "CAP ROW", PartKind::Cap, 0.50f, 0.70f, 0.14f, 0.20f, "Smooths the Hex Core power rails." },
        { "CHASSIS FAN", PartKind::Fan, 0.70f, 0.64f, 0.26f, 0.30f, "Keeps the Hex Core cool under load." },
        { "SIGNAL BUS", PartKind::Ribbon, 0.86f, 0.42f, 0.08f, 0.46f, "Carries audio around the Hex Core." }
    }, 7 }
};

inline juce::Rectangle<float> partRect(juce::Rectangle<float> interior, const HardwarePart& p)
{
    return { interior.getX() + p.x * interior.getWidth(), interior.getY() + p.y * interior.getHeight(),
             p.w * interior.getWidth(), p.h * interior.getHeight() };
}

// One special-looking piece per kind. hot = the hovered part gets the accent treatment.
inline void drawHardwarePart(juce::Graphics& g, const kt::ThemePalette& t, const HardwarePart& p,
                             juce::Rectangle<float> r, float phase, bool hot)
{
    const auto accent = kt::c(t.accent);
    const auto ink = kt::c(t.text);
    const auto muted = kt::c(t.muted);
    const auto peg = kt::c(t.peg);
    r.reduce(2.f, 2.f);
    if (r.getWidth() < 8.f || r.getHeight() < 8.f) return;

    const float glow = hot ? 1.f : 0.f;
    g.setColour(hot ? accent.withAlpha(0.30f) : kt::c(t.panel).withAlpha(0.72f));
    g.fillRoundedRectangle(r, 5.f);
    g.setColour(hot ? accent : kt::c(t.border).withAlpha(0.85f));
    g.drawRoundedRectangle(r, 5.f, hot ? 2.f : 1.f);

    switch (p.kind)
    {
        case PartKind::Board:
        {
            g.setColour(peg.darker(0.25f).withAlpha(hot ? 0.95f : 0.85f));
            g.fillRoundedRectangle(r.reduced(3.f), 4.f);
            g.setColour(accent.withAlpha(hot ? 0.9f : 0.5f));
            for (int i = 0; i < 4; ++i)
                g.drawLine(r.getX() + 6.f, r.getY() + 9.f + i * 6.f, r.getRight() - 12.f, r.getY() + 9.f + i * 6.f, 0.8f);
            auto chip = r.withSizeKeepingCentre(r.getWidth() * 0.34f, r.getHeight() * 0.4f);
            g.setColour(hot ? accent.withAlpha(0.85f) : kt::c(t.knob).withAlpha(0.95f));
            g.fillRoundedRectangle(chip, 2.f);
            g.setColour(ink.withAlpha(0.8f));
            g.setFont(kt::font(t, 7.5f, true));
            g.drawText("DSP", chip, juce::Justification::centred);
            for (int i = 0; i < 6; ++i)
            {
                g.setColour(accent.withAlpha(0.35f + glow * 0.3f));
                g.fillRect(r.getX() + 4.f + i * 5.f, r.getBottom() - 9.f, 2.5f, 5.f);
            }
            break;
        }
        case PartKind::Battery:
        {
            g.setColour(hot ? accent.withAlpha(0.5f) : kt::c(t.knob).withAlpha(0.55f));
            g.fillRoundedRectangle(r.reduced(4.f, 5.f), 4.f);
            const float cellTop = r.getY() + 5.f, cellBottom = r.getBottom() - 5.f;
            const float n = 4.f, bh = (cellBottom - cellTop - 6.f) / n;
            for (int i = 0; i < (int) n; ++i)
            {
                const float charge = 0.45f + 0.55f * (0.5f + 0.5f * std::sin(phase * 1.2f - (float) i));
                g.setColour(accent.withAlpha(0.25f + 0.55f * charge * (0.6f + glow * 0.4f)));
                g.fillRoundedRectangle(r.getX() + 7.f, cellTop + 3.f + (float) i * bh, r.getWidth() - 14.f, bh - 3.f, 2.f);
            }
            g.setColour(ink.withAlpha(0.85f));
            g.setFont(kt::font(t, 8.f, true));
            g.drawText("+", r.getX() + 3.f, r.getY(), 8.f, 10.f, juce::Justification::centredLeft);
            g.drawText("-", r.getRight() - 11.f, r.getY(), 8.f, 10.f, juce::Justification::centredLeft);
            break;
        }
        case PartKind::Cap:
        {
            const int n = r.getWidth() > 42.f ? 3 : 2;
            const float cw = (r.getWidth() - 4.f) / (float) n - 3.f;
            for (int i = 0; i < n; ++i)
            {
                auto c = juce::Rectangle<float>(r.getX() + 2.f + (float) i * (cw + 3.f), r.getY() + 4.f, cw, r.getHeight() - 8.f);
                g.setColour(hot ? accent.withAlpha(0.45f) : kt::c(t.knob).withAlpha(0.6f));
                g.fillRoundedRectangle(c, 3.f);
                g.setColour(hot ? accent : muted);
                g.fillRect(c.getX() + 1.f, c.getY() + 2.f, 2.f, c.getHeight() - 4.f);
            }
            break;
        }
        case PartKind::Ribbon:
        {
            const int lines = juce::jmin(9, (int) (r.getHeight() / 3.5f));
            for (int i = 0; i < lines; ++i)
            {
                juce::Path wire;
                const float y = r.getY() + 3.f + (r.getHeight() - 6.f) * (float) i / (float) juce::jmax(1, lines - 1);
                wire.startNewSubPath(r.getX() + 3.f, y);
                const float sway = (hot ? 4.f : 2.5f) * std::sin(phase * 1.5f + (float) i * 0.7f);
                wire.lineTo(r.getCentreX(), y + sway);
                wire.lineTo(r.getRight() - 3.f, y);
                g.setColour(accent.withAlpha(0.35f + glow * 0.4f));
                g.strokePath(wire, juce::PathStrokeType(1.1f));
            }
            break;
        }
        case PartKind::Fan:
        {
            const auto centre = r.getCentre();
            const float rad = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f - 3.f;
            g.setColour(kt::c(t.bg).withAlpha(0.6f));
            g.fillEllipse(centre.x - rad, centre.y - rad, rad * 2.f, rad * 2.f);
            for (int i = 0; i < 4; ++i)
            {
                juce::Path blade;
                const float a0 = phase * 2.4f + (float) i * (juce::MathConstants<float>::pi * 0.5f);
                blade.addCentredArc(centre.x, centre.y, rad, rad, 0.f, a0, a0 + 0.9f, true);
                g.setColour(accent.withAlpha(0.4f + glow * 0.35f));
                juce::Path stem; stem.startNewSubPath(centre.x, centre.y);
                stem.lineTo(centre.x + std::cos(a0) * rad, centre.y + std::sin(a0) * rad);
                g.strokePath(stem, juce::PathStrokeType(2.2f));
                juce::ignoreUnused(blade);
            }
            g.setColour(hot ? accent : kt::c(t.knob));
            g.fillEllipse(centre.x - rad * 0.22f, centre.y - rad * 0.22f, rad * 0.44f, rad * 0.44f);
            break;
        }
        case PartKind::Psu:
        {
            g.setColour(hot ? accent.withAlpha(0.35f) : kt::c(t.knob).withAlpha(0.5f));
            g.fillRoundedRectangle(r.reduced(4.f), 3.f);
            for (int i = 0; i < 5; ++i)
            {
                g.setColour(ink.withAlpha(0.5f + glow * 0.3f));
                g.fillRect(r.getX() + 6.f, r.getY() + 6.f + i * 4.f, r.getWidth() - 12.f, 1.4f);
            }
            break;
        }
        case PartKind::Ram:
        {
            const int sticks = r.getWidth() > 50.f ? 3 : 2;
            const float sw = (r.getWidth() - 6.f) / (float) sticks - 3.f;
            for (int i = 0; i < sticks; ++i)
            {
                auto s = juce::Rectangle<float>(r.getX() + 3.f + (float) i * (sw + 3.f), r.getY() + 4.f, sw, r.getHeight() - 8.f);
                g.setColour(hot ? accent.withAlpha(0.45f) : peg.withAlpha(0.7f));
                g.fillRoundedRectangle(s, 2.f);
                for (int pin = 0; pin < 4; ++pin)
                {
                    g.setColour(ink.withAlpha(0.55f));
                    g.fillRect(s.getX() + 2.f, s.getBottom() - 5.f - pin * 3.5f, s.getWidth() - 4.f, 1.2f);
                }
            }
            break;
        }
        case PartKind::Port:
        {
            auto slotR = r.reduced(4.f);
            g.setColour(kt::c(t.bg).withAlpha(0.85f));
            g.fillRoundedRectangle(slotR, slotR.getWidth() * 0.5f);
            g.setColour(hot ? accent : muted);
            const int pins = juce::jmax(2, (int) (slotR.getHeight() / 7.f));
            for (int i = 0; i < pins; ++i)
                g.fillRect(slotR.getCentreX() - 1.2f, slotR.getY() + 3.f + (float) i * 7.f, 2.4f, 3.5f);
            break;
        }
        case PartKind::Antenna:
        {
            juce::Path zig;
            zig.startNewSubPath(r.getX() + 2.f, r.getBottom() - 3.f);
            const float steps = 5.f;
            for (int i = 1; i <= (int) steps; ++i)
            {
                const float fx = r.getX() + 2.f + (r.getWidth() - 4.f) * (float) i / steps;
                const float fy = r.getBottom() - 3.f - (r.getHeight() - 6.f) * (float) i / steps * ((i % 2 == 0) ? 0.75f : 1.f);
                zig.lineTo(fx, fy);
            }
            g.setColour(accent.withAlpha(0.5f + glow * 0.4f));
            g.strokePath(zig, juce::PathStrokeType(1.6f));
            break;
        }
        case PartKind::Header:
        {
            const int rows = 2, cols = juce::jmax(3, (int) (r.getWidth() / 8.f));
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col)
                {
                    g.setColour((row + col) % 2 == 0 ? accent.withAlpha(0.55f + glow * 0.3f) : muted.withAlpha(0.6f));
                    g.fillRect(r.getX() + 3.f + (float) col * 7.f, r.getY() + 4.f + (float) row * 6.f, 3.f, 4.f);
                }
            break;
        }
        case PartKind::Drive:
        {
            g.setColour(peg.withAlpha(0.75f));
            g.fillRoundedRectangle(r.reduced(4.f), 3.f);
            const auto disc = r.withSizeKeepingCentre(r.getHeight() - 14.f, r.getHeight() - 14.f);
            const auto c = disc.getCentre();
            g.setColour(kt::c(t.bg).withAlpha(0.8f));
            g.fillEllipse(disc);
            g.setColour(accent.withAlpha(0.6f + glow * 0.3f));
            const float a = phase * 1.8f;
            g.drawLine(c.x, c.y, c.x + std::cos(a) * disc.getWidth() * 0.45f, c.y + std::sin(a) * disc.getWidth() * 0.45f, 1.4f);
            g.fillEllipse(c.x - 2.f, c.y - 2.f, 4.f, 4.f);
            break;
        }
    }

    g.setColour(hot ? accent : muted);
    g.setFont(kt::font(t, 7.5f, true));
    g.drawText(juce::String(p.name), r, juce::Justification::centredTop);
}
}
