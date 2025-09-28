#!/usr/bin/env python3

from liblk.image import LkImage
from liblk.structures import ImageHeader
from liblk.structures.partition import LkPartition
import argparse

def create_partition(data, original_header, load_addr=0xFFFFFFFF, mode=0xFFFFFFFF):
    header = ImageHeader.from_buffer_copy(bytes(original_header))
    setattr(header, 'name', 'lk')
    setattr(header, 'data_size', len(data))
    setattr(header, 'memory_address', load_addr)
    setattr(header, 'mode', mode)
    
    padded_data = data
    if len(data) % 16:
        padding_size = 16 - (len(data) % 16)
        padded_data = data + b'\x00' * padding_size
        setattr(header, 'data_size', len(padded_data))
    
    partition = LkPartition(
        header=header,
        data=padded_data,
        end_offset=0,
        lk_address=load_addr if load_addr != 0xFFFFFFFF else None
    )
    
    return partition

def replace_partition(lk_path, uboot_path, output_path, load_addr=0xFFFFFFFF, mode=0xFFFFFFFF):
    lk_image = LkImage(lk_path)
    
    print("Original image has %d partitions:" % len(lk_image))
    for name, partition in lk_image.partitions.items():
        print("  - %s: %d bytes" % (name, len(partition)))
    print("")

    if 'lk' not in lk_image.partitions:
        raise ValueError("No 'lk' partition found in the image")
    
    with open(uboot_path, 'rb') as f:
        uboot_data = f.read()
    
    print("U-Boot size: %d bytes" % len(uboot_data))
    
    original_lk = lk_image.partitions['lk']
    print("Original LK partition has %d certificates" % len(original_lk.certs))
    
    uboot_partition = create_partition(uboot_data, original_lk.header, load_addr, mode)
    uboot_partition.certs = original_lk.certs.copy()
    
    lk_image.partitions['lk'] = uboot_partition
    rebuild_image_contents(lk_image)
    
    print("Saving modified image to: %s" % output_path)
    lk_image.save(output_path)

def rebuild_image_contents(lk_image):
    new_contents = bytearray()
    partition_names = list(lk_image.partitions.keys())
    
    for i, (name, partition) in enumerate(lk_image.partitions.items()):
        partition.header.image_list_end = 1 if i == len(partition_names) - 1 else 0
        partition.end_offset = len(new_contents) + partition.header.size + partition.header.data_size
        
        alignment = partition.header.alignment if partition.header.is_extended else 8
        if alignment and partition.end_offset % alignment:
            partition.end_offset += alignment - (partition.end_offset % alignment)
        
        new_contents.extend(bytes(partition))
    
    lk_image.contents = new_contents

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('lk_image', help='Path to the original LK image')
    parser.add_argument('uboot_bin', help='Path to the U-Boot binary')
    parser.add_argument('output', help='Path for the modified LK image')
    parser.add_argument('--addr', type=lambda x: int(x, 0), default=0xFFFFFFFF, 
                       help='Load address (default: 0xFFFFFFFF)')
    parser.add_argument('--mode', type=lambda x: int(x, 0), default=0xFFFFFFFF, 
                       help='Addressing mode (default: 0xFFFFFFFF)')
    
    args = parser.parse_args()
    
    try:
        print("")
        replace_partition(args.lk_image, args.uboot_bin,
                          args.output, args.addr, args.mode)
    except Exception as e:
        print("Error: %s" % e)
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())