"""Nuerolynx on-device application-library taxonomy.

The upstream FAP category remains part of each application's manifest. This
module only adds a second directory level when the firmware resource bundle is
assembled, keeping third-party manifests easy to update while making the
128x64 Applications browser practical to navigate.
"""

from fbt.appmanifest import FlipperAppType


# These applications update themselves into their historical category root.
# Leaving them in place prevents an in-app update from creating a duplicate.
_KEEP_AT_CATEGORY_ROOT = {
    "flip_downloader",
    "flip_library",
    "flip_map",
    "flip_social",
    "flip_telegram",
    "flip_trader",
    "flip_weather",
    "flip_wifi",
    "flip_world",
    "free_roam",
    "web_crawler",
}


_NLX_APP_SUBFOLDERS = {
    "Games": {
        "Arcade": {
            "air_arkanoid",
            "arkanoid",
            "asteroids",
            "bomberduck",
            "doom",
            "flappy_bird",
            "flipper_pong",
            "geometry_dash",
            "jetpack_joyride",
            "laser_tag",
            "pinball0",
            "quadrastic",
            "t_rex_runner",
            "yapinvaders",
            "zombiez",
        },
        "Board": {
            "4inrow",
            "blackjack",
            "checkers",
            "chess",
            "multi_dice",
            "reversi",
            "slotmachine",
            "solitaire",
            "tictactoe",
            "umpire_indicator",
            "videopoker",
            "yatzee",
        },
        "Puzzle": {
            "air_labyrinth",
            "color_guess",
            "game15",
            "game_2048",
            "minesweeper_redux",
            "roots_of_life",
            "rubiks_cube_scrambler",
            "simon_says",
            "snake20",
            "tetris",
        },
        "Strategy-Sim": {
            "gameoflife",
            "heap_defence",
            "scorched_tanks",
            "tama_p1",
            "tanks",
        },
    },
    "GPIO": {
        "Boards": {
            "coleco",
            "flipboard_blinky",
            "flipboard_keyboard",
            "flipboard_signal",
            "flipboard_simon",
            "flipper_atomicdiceroller",
            "malveke_gb_cartridge",
            "malveke_gb_emulator",
            "malveke_gba_cartridge",
            "malveke_pin_test",
            "pokemon",
            "vgm_air_mouse",
            "video_game_module_tool",
        },
        "Cameras": {
            "camera_suite",
            "malveke_gb_link_camera",
            "malveke_gb_live_camera",
            "malveke_gb_photo",
            "mayhem_camera",
            "mayhem_nannycam",
            "mayhem_qrcode",
            "timelapse",
        },
        "Diagnostics": {
            "can_commander",
            "eth_troubleshooter",
            "flipperscope",
            "gpio",
            "gpio_badge",
            "gpio_controller",
            "gpio_explorer_app",
            "gpio_logic_analyzer",
            "gpio_reader_a",
            "gpio_reader_b",
            "i2ctools",
            "ina_meter",
            "uart_terminal",
            "wire_tester",
        },
        "Network": {
            "esp8266_ifttt_virtual_button",
            "nearby_files",
            "wardriver",
            "wifi_scanner",
        },
        "Programming": {
            "avr_isp",
            "dap_link",
            "esp_flasher",
            "flip_tdi",
            "flipper_spi_terminal",
            "spi_mem_manager",
            "swd_probe",
            "wii_ec_anal",
        },
        "Radio": {
            "fm_radio",
            "fmtx_app",
            "longwave_clock",
            "nrf24batch",
            "nrf24channelscanner",
            "nrf24scan",
            "nrf24sniff",
            "signal_generator",
        },
        "Security": {
            "blackhat",
            "esp32_wifi_marauder",
            "esp8266_deauther",
            "esp8266_wifi_deauther_v2",
            "evil_portal",
            "ghost_esp",
            "gpio_sentry_safe",
            "magspoof",
            "mayhem_marauder",
            "nrf24mousejacker",
        },
        "Sensors": {
            "co2_logger",
            "flipper_geiger",
            "gps_nmea",
            "hc_sr04",
            "lightmeter",
            "mayhem_motion",
            "radar_scanner",
            "unitemp",
            "uv_meter_as7331",
        },
        "Utilities": {
            "air_mouse",
            "flashlight",
            "mayhem_morseflash",
        },
    },
    "Infrared": {
        "Remotes": {
            "flipper_xremote",
            "hitachi_ac_remote",
            "ir_remote",
            "midea_ac_remote",
            "mitsubishi_ac_remote",
            "xbox_controller",
            "xremote",
        },
        "Tools": {
            "flame_rng",
            "ir_intervalometer",
            "ir_scope",
            "lidar_emulator",
        },
    },
    "Media": {
        "Audio": {
            "bpm_tapper",
            "flizzer_tracker",
            "metronome",
            "music_player",
            "text2sam",
            "tuning_fork",
            "usb_midi",
            "wav_player",
        },
        "Utilities": {"morse_code"},
        "Visual": {
            "etch",
            "fmatrix",
            "fzspground",
            "image_viewer",
            "paint",
            "video_player",
        },
    },
    "NFC": {
        "Credentials": {
            "ami_tool",
            "cyborg_detector",
            "metroflip",
            "mfc_editor",
            "nfc_login",
            "passy",
            "picopass",
            "saflip",
            "seader",
            "seos",
            "weebo",
        },
        "Lab": {
            "iso15693_nfc_writer",
            "mfkey",
            "mifare_fuzzer",
            "nfc_apdu_runner",
            "nfc_magic",
            "ulc_brute",
            "ulc_relay",
            "ulcfkey",
        },
        "Utilities": {
            "nfc_eink",
            "nfc_maker",
            "nfc_playlist",
        },
    },
    "Sub-GHz": {
        "Analyze": {
            "proto_pirate",
            "protoview",
            "radio_scanner",
            "smart_meter_monitor",
            "spectrum_analyzer",
            "sub_analyzer",
            "tpms",
            "weather_station",
        },
        "Automate": {
            "fmf_to_sub",
            "subghz_playlist",
            "subghz_playlist_creator",
            "subghz_remote",
            "subghz_scheduler",
        },
        "Communicate": {
            "esubghz_chat",
            "flipper_share",
            "hc11_modem",
            "meal_pager",
            "pocsag_pager",
        },
        "Lab": {
            "chief_cooker",
            "rolling_flaws",
            "subghz_bruteforcer",
        },
    },
    "Tools": {
        "Calculate": {
            "calculator",
            "counter",
            "multi_converter",
            "programmercalc",
            "resistors",
            "voltcalc_app",
        },
        "Field": {
            "barcode_app",
            "can_tools",
            "dtmf_dolphin",
            "flipper_wedge",
            "qrcode",
            "quac",
            "tone_gen",
        },
        "Security": {
            "caesar_cipher",
            "combo_cracker",
            "flip_crypt",
            "key_copier",
            "nfc_rfid_detector",
            "nlx_credential_suite",
            "passgen",
            "totp",
        },
        "System": {
            "cntdown_tim",
            "flipbip",
            "flipp_pomodoro",
            "hex_editor",
            "hex_viewer",
            "iconedit",
            "nightstand",
            "tasks",
            "text_viewer",
            "upython",
        },
    },
}


def get_nlx_app_library_category(app):
    """Return the package directory for an external FAP."""

    category = app.fap_category
    if app.apptype != FlipperAppType.EXTERNAL or not category:
        return category
    if app.appid in _KEEP_AT_CATEGORY_ROOT:
        return category

    subfolders = _NLX_APP_SUBFOLDERS.get(category)
    if not subfolders:
        return category

    for subfolder, app_ids in subfolders.items():
        if app.appid in app_ids:
            return f"{category}/{subfolder}"
    return f"{category}/Other"


def _validate_taxonomy():
    seen = set()
    for category, subfolders in _NLX_APP_SUBFOLDERS.items():
        for subfolder, app_ids in subfolders.items():
            overlap = seen.intersection(app_ids)
            if overlap:
                raise ValueError(
                    f"Duplicate NLX app taxonomy entries in {category}/{subfolder}: "
                    f"{sorted(overlap)}"
                )
            seen.update(app_ids)


_validate_taxonomy()
