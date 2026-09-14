#!/usr/bin/env python3

"""Generate the small, pixel-perfect Nuerolynx firmware identity assets."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]

# Hand-cleaned pixel reductions traced from the original 46 x 49 Nuerolynx
# artwork. Each row is a final device pixel grid; no runtime scaling is used.
LOGO_14 = (
    ".....###......",
    "....##..##....",
    "..#.##..#.#...",
    ".#..##..#..##.",
    "...#.#..#.#...",
    ".#..#....#.#..",
    "...#.####.....",
    "....#.#..#.#..",
    "...##...###...",
    ".#...#..#.#.#.",
    ".#..#.##.#..#.",
    "..#..#.##.#...",
    "....#....#....",
    "......##......",
)

# User-approved full-detail menu reduction. The menu renderer supplies the
# surrounding one-pixel clearance, allowing all 19 native pixels to preserve
# the stepped cube, side traces, centre fork, spine, and pointed base.
LOGO_MENU_19 = (
    "........###........",
    "......###.###......",
    ".....##.....##.....",
    "..##..##...##..##..",
    "###...##...##...###",
    "......#.....#......",
    "...##..##.##..##...",
    ".##..#...#...#..##.",
    "..###.#.....#.###..",
    ".#..##..###..##..#.",
    "..#..#.#.#.#.#..#..",
    "...#.##..#..##.#...",
    "##.#..##.#.##..#.##",
    "##.##..#.#.#..##.##",
    ".##..#.#.#.#.#..##.",
    "..##...#.#.#...##..",
    "....##...#...##....",
    "......##.#.##......",
    "........###........",
)

# Control Center tiles are 28 x 28 and center icons using their real size.
# This is the exact native-grid reduction of the user-approved 250 x 250
# Nuerolynx artwork. Each 10 x 10 source block becomes one device pixel. The
# resulting art is perfectly mirrored and its blank outer ring leaves the
# requested one-pixel clearance from the Control Center tile border.
LOGO_CONTROL_CENTER = (
    ".........................",
    "...........###...........",
    ".........###.###.........",
    "........##.....##........",
    ".....#.##.......##.#.....",
    "...###..###...###..###...",
    ".###....###...###....###.",
    "........#.......#........",
    "....##...##...##...##....",
    "..#...##...###...##...#..",
    "..###...#.......#...###..",
    "....###..#.#.#.#..###....",
    "..#...##.#.###.#.##...#..",
    "...##..#.#..#..#.#..##...",
    ".....#.#.#..#..#.#.#.....",
    ".##..#.###..#..###.#..##.",
    ".##..#...##.#.##...#..##.",
    ".##..##...#.#.#...##..##.",
    "..##...##.#.#.#.##...##..",
    "...###...##.#.##...###...",
    ".....##.....#.....##.....",
    ".......##...#...##.......",
    ".........##.#.##.........",
    "...........###...........",
    ".........................",
)

WORDMARK_GLYPHS = {
    "N": (
        "#...#",
        "##..#",
        "##..#",
        "#.#.#",
        "#..##",
        "#..##",
        "#...#",
    ),
    "U": (
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        ".###.",
    ),
    "E": (
        "#####",
        "#....",
        "#....",
        "####.",
        "#....",
        "#....",
        "#####",
    ),
    "R": (
        "####.",
        "#...#",
        "#...#",
        "####.",
        "#.#..",
        "#..#.",
        "#...#",
    ),
    "O": (
        ".###.",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        "#...#",
        ".###.",
    ),
    "L": (
        "#....",
        "#....",
        "#....",
        "#....",
        "#....",
        "#....",
        "#####",
    ),
    "Y": (
        "#...#",
        "#...#",
        ".#.#.",
        "..#..",
        "..#..",
        "..#..",
        "..#..",
    ),
    "X": (
        "#...#",
        "#...#",
        ".#.#.",
        "..#..",
        ".#.#.",
        "#...#",
        "#...#",
    ),
}


def image_from_rows(rows: tuple[str, ...], width: int, height: int) -> Image.Image:
    if len(rows) != height or any(len(row) != width for row in rows):
        raise ValueError("Pixel-art dimensions do not match the requested image size")

    image = Image.new("1", (width, height), 1)
    pixels = image.load()
    for y, row in enumerate(rows):
        for x, pixel in enumerate(row):
            if pixel == "#":
                pixels[x, y] = 0
    return image


def save_1bit(image: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image.convert("1").save(path, optimize=True)


def mirror_from_left(image: Image.Image, axis_x2: int) -> Image.Image:
    """Mirror the left half across an axis expressed as twice its x value."""
    source = image.convert("1")
    mirrored = source.copy()
    for y in range(source.height):
        for x in range(source.width):
            mirror_x = axis_x2 - x
            if x * 2 > axis_x2 and 0 <= mirror_x < source.width:
                mirrored.putpixel((x, y), source.getpixel((mirror_x, y)))
    return mirrored


def validate_horizontal_symmetry(image: Image.Image, axis_x2: int, name: str) -> None:
    pixels = image.convert("1")
    for y in range(pixels.height):
        for x in range(pixels.width):
            mirror_x = axis_x2 - x
            if 0 <= mirror_x < pixels.width:
                if pixels.getpixel((x, y)) != pixels.getpixel((mirror_x, y)):
                    raise ValueError(f"{name} is not perfectly symmetrical")


def make_logo_14() -> Image.Image:
    return image_from_rows(LOGO_14, 14, 14)


def make_menu_logo_19() -> Image.Image:
    if any(row != row[::-1] for row in LOGO_MENU_19):
        raise ValueError("Menu logo is not perfectly symmetrical")
    return image_from_rows(LOGO_MENU_19, 19, 19)


def validate_control_center_logo() -> None:
    points = {
        (x, y)
        for y, row in enumerate(LOGO_CONTROL_CENTER)
        for x, pixel in enumerate(row)
        if pixel == "#"
    }

    if points != {(24 - x, y) for x, y in points}:
        raise ValueError("Control Center logo is not perfectly symmetrical")
    if any(x in (0, 24) or y in (0, 24) for x, y in points):
        raise ValueError("Control Center logo lost its one-pixel clear border")
    remaining = set(points)
    components = 0
    while remaining:
        components += 1
        pending = [remaining.pop()]
        while pending:
            x, y = pending.pop()
            neighbors = {
                (x + dx, y + dy)
                for dy in (-1, 0, 1)
                for dx in (-1, 0, 1)
                if dx or dy
            }
            connected = neighbors & remaining
            remaining -= connected
            pending.extend(connected)
    if components != 6:
        raise ValueError(
            f"Control Center logo has {components} connected groups instead of 6"
        )


def make_control_center_logo() -> Image.Image:
    validate_control_center_logo()
    return image_from_rows(LOGO_CONTROL_CENTER, 25, 25)


def replace_menu_icons() -> None:
    legacy_logo = make_logo_14()
    legacy_icon_dirs = (
        ROOT / "assets/icons/MainMenu/Momentum_14",
        ROOT / "assets/packs/Nuerolynx/Icons/MainMenu/Momentum_14",
    )
    for icon_dir in legacy_icon_dirs:
        for frame in sorted(icon_dir.glob("frame_*.png")):
            save_1bit(legacy_logo, frame)

    # Dedicated, user-approved artwork for the live Nuerolynx menu entry. The
    # old animation remains solely for saved-menu/pack compatibility.
    menu_logo = make_menu_logo_19()
    for path in (
        ROOT / "assets/icons/MainMenu/Nuerolynx_19x19.png",
        ROOT / "assets/packs/Nuerolynx/Icons/MainMenu/Nuerolynx_19x19.png",
    ):
        save_1bit(menu_logo, path)


def replace_control_center_icons() -> None:
    logo = make_control_center_logo()
    for path in (
        ROOT / "assets/icons/ControlCenter/CC_Nuerolynx_25x25.png",
        ROOT / "assets/packs/Nuerolynx/Icons/ControlCenter/CC_Nuerolynx_25x25.png",
        ROOT / "assets/icons/ControlCenter/CC_Momentum_16x16.png",
        ROOT / "assets/packs/Nuerolynx/Icons/ControlCenter/CC_Momentum_16x16.png",
    ):
        save_1bit(logo, path)


def replace_updater_brand() -> None:
    # The approved 25 x 25 native-pixel logo is used without scaling. Centering
    # it on the existing 32 x 40 updater canvas leaves clear space on all sides.
    logo = make_control_center_logo()
    updater_icon = Image.new("1", (32, 40), 1)
    updater_icon.paste(logo, (3, 7))
    if updater_icon.crop((3, 7, 28, 32)).tobytes() != logo.tobytes():
        raise ValueError("Updater logo does not exactly match the approved pixel grid")
    save_1bit(updater_icon, ROOT / "assets/icons/Update/Updating_32x40.png")

    # Draw the complete name on a fixed 5 x 7 grid. Each glyph is separated by
    # one deliberately empty column so no two letters can touch on the display.
    word = "NUEROLYNX"
    glyph_width = 5
    glyph_gap = 1
    word_width = len(word) * glyph_width + (len(word) - 1) * glyph_gap
    wordmark = Image.new("1", (62, 15), 1)
    x = (wordmark.width - word_width) // 2
    y = (wordmark.height - 7) // 2
    blank_columns = []
    for index, letter in enumerate(word):
        x += draw_glyph(wordmark, x, y, WORDMARK_GLYPHS[letter])
        if index != len(word) - 1:
            blank_columns.append(x)
            x += glyph_gap

    if any(
        wordmark.getpixel((blank_x, row)) == 0
        for blank_x in blank_columns
        for row in range(wordmark.height)
    ):
        raise ValueError("Updater wordmark lost a required one-pixel letter gap")
    save_1bit(wordmark, ROOT / "assets/icons/Update/Updating_Logo_62x15.png")


def replace_firstboot_slideshow() -> None:
    """Remove the remaining device-facing Momentum artwork from first boot."""
    slideshow_dir = ROOT / "assets/slideshow/firstboot"
    wordmark = Image.open(ROOT / "assets/icons/Update/Updating_Logo_62x15.png").convert("1")
    compact_logo = make_control_center_logo()

    # Welcome page: retain the native text and NEXT button, but replace the
    # complete legacy lockup with the approved NLX logo and spaced wordmark.
    frame = Image.open(slideshow_dir / "frame_00.png").convert("1")
    frame.paste(1, (0, 11, 128, 45))
    frame.paste(wordmark, (6, 16))
    frame.paste(compact_logo, (72, 12))
    save_1bit(frame, slideshow_dir / "frame_00.png")

    # The third Quick Controls tile used the old Momentum M. Replace only its
    # interior, preserving the original tile border and tutorial layout.
    frame = Image.open(slideshow_dir / "frame_01.png").convert("1")
    frame.paste(1, (96, 4, 125, 28))
    frame.paste(make_logo_14(), (103, 8))
    save_1bit(frame, slideshow_dir / "frame_01.png")

    # Rename the settings entry shown in the tutorial. The wordmark is drawn
    # on its native grid and retains at least one clear column between glyphs.
    frame = Image.open(slideshow_dir / "frame_04.png").convert("1")
    frame.paste(1, (20, 17, 128, 29))
    frame.paste(wordmark, (35, 15))
    save_1bit(frame, slideshow_dir / "frame_04.png")

    # Replace the large legacy M on the feature page with the full-detail,
    # symmetrical 46 x 49 Nuerolynx trace cube used by Passport.
    frame = Image.open(slideshow_dir / "frame_05.png").convert("1")
    frame.paste(1, (0, 0, 50, 51))
    full_logo = Image.open(
        ROOT / "assets/packs/Nuerolynx/Icons/Passport/passport_happy_46x49.png"
    ).convert("1")
    validate_horizontal_symmetry(full_logo, 44, "First-boot feature logo")
    frame.paste(full_logo, (2, 1))
    save_1bit(frame, slideshow_dir / "frame_05.png")

    # Final first-boot page: retain the explanatory copy and OK button while
    # replacing the entire Momentum footer lockup with Nuerolynx.
    frame = Image.open(slideshow_dir / "frame_06.png").convert("1")
    frame.paste(1, (0, 30, 82, 64))
    frame.paste(wordmark, (8, 43))
    save_1bit(frame, slideshow_dir / "frame_06.png")


def replace_update_result_slideshow() -> None:
    """Brand the post-update success page without changing updater behavior."""
    result_path = ROOT / "assets/slideshow/update_default/frame_00.png"
    frame = Image.open(result_path).convert("1")
    wordmark = Image.open(ROOT / "assets/icons/Update/Updating_Logo_62x15.png").convert("1")
    logo = make_control_center_logo()

    # The lower OK/UPDATED result remains native. Everything above it was a
    # Momentum lockup, so replace that entire region with approved NLX art.
    frame.paste(1, (0, 0, 128, 39))
    frame.paste(logo, (7, 7))
    frame.paste(wordmark, (42, 10))
    save_1bit(frame, result_path)


def replace_matrix_logo() -> None:
    # The matrix animation uses one fixed 32 x 37 mark while only the rain
    # changes between frames. Mirror its left half on the native pixel grid so
    # every frame carries the exact same, perfectly symmetrical artwork.
    logo_box = (8, 17, 40, 54)
    animation_dir = ROOT / "assets/packs/Nuerolynx/Anims/Nuerolynx_128x64"
    for frame_path in sorted(animation_dir.glob("frame_*.png")):
        frame = Image.open(frame_path).convert("1")
        logo = mirror_from_left(frame.crop(logo_box), 31)
        validate_horizontal_symmetry(logo, 31, f"Matrix logo in {frame_path.name}")
        frame.paste(logo, logo_box[:2])
        save_1bit(frame, frame_path)


def replace_passport_logos() -> None:
    # The Passport card reserves a 46 x 49 canvas. Its one-pixel centre spine
    # sits on x=22, so the visible artwork mirrors around that native column;
    # the final unused canvas column remains blank.
    passport_dir = ROOT / "assets/packs/Nuerolynx/Icons/Passport"
    for logo_path in sorted(passport_dir.glob("passport_*_46x49.png")):
        logo = mirror_from_left(Image.open(logo_path), 44)
        validate_horizontal_symmetry(logo, 44, f"Passport logo in {logo_path.name}")
        save_1bit(logo, logo_path)


def draw_glyph(image: Image.Image, x: int, y: int, rows: tuple[str, ...]) -> int:
    pixels = image.load()
    width = len(rows[0])
    for row_y, row in enumerate(rows):
        if len(row) != width:
            raise ValueError("Glyph rows must have equal widths")
        for row_x, pixel in enumerate(row):
            if pixel == "#":
                pixels[x + row_x, y + row_y] = 0
    return width


def replace_passport_brand() -> None:
    passport_path = ROOT / "assets/icons/Passport/passport_128x64.png"
    passport = Image.open(passport_path).convert("1")

    # Clear the old 5 px MNTM wordmark while preserving the surrounding frame.
    for y in range(54, 59):
        for x in range(11, 38):
            passport.putpixel((x, y), 1)

    glyphs = {
        "N": ("#...#", "##..#", "#.#.#", "#..##", "#...#"),
        "L": ("#...", "#...", "#...", "#...", "####"),
        "X": ("#...#", ".#.#.", "..#..", ".#.#.", "#...#"),
    }
    x = 15
    for letter in "NLX":
        # One blank pixel column is kept between every letter.
        x += draw_glyph(passport, x, 54, glyphs[letter]) + 1

    save_1bit(passport, passport_path)
    save_1bit(
        passport,
        ROOT / "assets/packs/Nuerolynx/Icons/Passport/passport_128x64.png",
    )


def main() -> None:
    replace_menu_icons()
    replace_control_center_icons()
    replace_updater_brand()
    replace_matrix_logo()
    replace_passport_logos()
    replace_passport_brand()
    replace_firstboot_slideshow()
    replace_update_result_slideshow()


if __name__ == "__main__":
    main()
