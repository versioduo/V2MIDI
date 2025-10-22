#pragma once

// Controller (CC) numbers and Channel Mode Messages.
namespace V2MIDI::CC {
  enum {
    // MSB Controller Data.
    BankSelect       = 0,
    ModulationWheel  = 1,
    BreathController = 2,
    Controller3      = 3,
    FootController   = 4,
    PortamentoTime   = 5,
    DataEntry        = 6, // RPN, NRPN value.
    ChannelVolume    = 7,
    Balance          = 8,
    Controller9      = 9,
    Pan              = 10,
    Expression       = 11,
    EffectControl1   = 12,
    EffectControl2   = 13,
    Controller14     = 14,
    Controller15     = 15,
    GeneralPurpose1  = 16,
    GeneralPurpose2  = 17,
    GeneralPurpose3  = 18,
    GeneralPurpose4  = 19,
    Controller20     = 20,
    Controller21     = 21,
    Controller22     = 22,
    Controller23     = 23,
    Controller24     = 24,
    Controller25     = 25,
    Controller26     = 26,
    Controller27     = 27,
    Controller28     = 28,
    Controller29     = 29,
    Controller30     = 30,
    Controller31     = 31,

    // LSB for controllers 0 to 31.
    ControllerLSB       = 32,
    BankSelectLSB       = ControllerLSB + BankSelect,
    ModulationWheelLSB  = ControllerLSB + ModulationWheel,
    BreathControllerLSB = ControllerLSB + BreathController,
    Controller3LSB      = ControllerLSB + Controller3,
    FootControllerLSB   = ControllerLSB + FootController,
    PortamentoTimeLSB   = ControllerLSB + PortamentoTime,
    DataEntryLSB        = ControllerLSB + DataEntry,
    ChannelVolumeLSB    = ControllerLSB + ChannelVolume,
    BalanceLSB          = ControllerLSB + Balance,
    Controller9LSB      = ControllerLSB + Controller9,
    PanLSB              = ControllerLSB + Pan,
    ExpressionLSB       = ControllerLSB + Expression,
    EffectControl1LSB   = ControllerLSB + EffectControl1,
    EffectControl2LSB   = ControllerLSB + EffectControl2,
    Controller14LSB     = ControllerLSB + Controller14,
    Controller15LSB     = ControllerLSB + Controller15,
    GeneralPurpose1LSB  = ControllerLSB + GeneralPurpose1,
    GeneralPurpose2LSB  = ControllerLSB + GeneralPurpose2,
    GeneralPurpose3LSB  = ControllerLSB + GeneralPurpose3,
    GeneralPurpose4LSB  = ControllerLSB + GeneralPurpose4,
    Controller20LSB     = ControllerLSB + Controller20,
    Controller21LSB     = ControllerLSB + Controller21,
    Controller22LSB     = ControllerLSB + Controller22,
    Controller23LSB     = ControllerLSB + Controller23,
    Controller24LSB     = ControllerLSB + Controller24,
    Controller25LSB     = ControllerLSB + Controller25,
    Controller26LSB     = ControllerLSB + Controller26,
    Controller27LSB     = ControllerLSB + Controller27,
    Controller28LSB     = ControllerLSB + Controller28,
    Controller29LSB     = ControllerLSB + Controller29,
    Controller30LSB     = ControllerLSB + Controller30,
    Controller31LSB     = ControllerLSB + Controller31,

    // Single-byte Controllers.
    SustainPedal      = 64,
    Portamento        = 65,
    Sostenuto         = 66,
    SoftPedal         = 67,
    LegatoPedal       = 68,
    Hold2             = 69,
    SoundController1  = 70, // Sound Variation
    SoundController2  = 71, // Timber / Harmonic Intensity
    SoundController3  = 72, // Release Time
    SoundController4  = 73, // Attack Time
    SoundController5  = 74, // Brightness
    SoundController6  = 75, // Decay Time
    SoundController7  = 76, // Vibrato Rate
    SoundController8  = 77, // Vibrato Depth
    SoundController9  = 78, // Vibrato Delay
    SoundController10 = 79,
    GeneralPurpose5   = 80, // Decay
    GeneralPurpose6   = 81, // High Pass Filter Frequency
    GeneralPurpose7   = 82,
    GeneralPurpose8   = 83,
    PortamentoControl = 84,
    Controller85      = 85,
    Controller86      = 86,
    Controller87      = 87,
    VelocityPrefix    = 88,
    Controller89      = 89,
    Controller90      = 90,
    Effects1          = 91, // Reverb Send
    Effects2          = 92, // Tremolo Depth
    Effects3          = 93, // Chorus Send
    Effects4          = 94, // Celeste Depth
    Effects5          = 95, // Phaser Depth

    // Non-registered, Registered Parameter Numbers
    DataIncrement = 96, // Step == 1, ignore the value (RP-018)
    DataDecrement = 97,
    NRPNLSB       = 98, // Select NRPN
    NRPNMSB       = 99,
    RPNLSB        = 100, // Select RPN
    RPNMSB        = 101,

    Controller102 = 102,
    Controller103 = 103,
    Controller104 = 104,
    Controller105 = 105,
    Controller106 = 106,
    Controller107 = 107,
    Controller108 = 108,
    Controller109 = 109,
    Controller110 = 110,
    Controller111 = 111,
    Controller112 = 112,
    Controller113 = 113,
    Controller114 = 114,
    Controller115 = 115,
    Controller116 = 116,
    Controller117 = 117,
    Controller118 = 118,
    Controller119 = 119,

    // Channel Mode Message
    AllSoundOff         = 120,
    ResetAllControllers = 121,
    LocalControl        = 122,
    AllNotesOff         = 123,
    OmniModeOff         = 124,
    OmniModeOn          = 125,
    MonoModeOn          = 126,
    PolyModeOn          = 127
  };
}
