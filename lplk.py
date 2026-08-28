# let's patch lk! - minimal version for u-boot

from liblk import LkImage

# Snippet credits: https://github.com/R0rt1z2/liblk/blob/master/examples/apply_binary_patch.py
def validate_patch_input(needle: str, patch: str) -> tuple[bytes, bytes]:
    """
    Validate and convert patch inputs.

    Args:
        needle: Byte sequence to replace (hex string)
        patch: Replacement byte sequence (hex string)

    Returns:
        Tuple of needle and patch as bytes
    """
    try:
        needle_bytes = bytes.fromhex(needle)
        patch_bytes = bytes.fromhex(patch)
    except ValueError as e:
        raise ValueError(f'Invalid hex input: {e}')

    return needle_bytes, patch_bytes

def patch(lk: LkImage, target: str, patch: str):
    try:
        target_bytes, patch_bytes = validate_patch_input(target, patch)
        
        lk.apply_patch(target_bytes, patch_bytes)

    except NeedleNotFoundException as e:
        log.error(f'Bytes for patch not found! ({e})')
        raise ValueError(f'Bytes to patch were not found!')

def disable_lk_self_verify(lk: LkImage):
    # NOTE: Without this, the device will BRICK!!!
    # Disable image verification for every partition in lk, except dtbo and main_dtb
    # Credits for the discovery: https://github.com/R0rt1z2/fenrir/blob/main/injector/devices.py
    lk_exec = lk.partitions.__len__() - 2

    for n in range(lk_exec):
        print(f"Disabling verification in {list(lk.partitions.keys())[n]}")
        patch(lk, '000100b4fd7bbfa9', '00008052c0035fd6')

def main():
    lk = LkImage("lk.img")

    # 0. Booting tweaks
    disable_lk_self_verify(lk)

    lk.save("lplk_out.img")
    
if __name__=="__main__":
    main()
