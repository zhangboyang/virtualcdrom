#!/usr/bin/env python3

import sys
import os
import gc
import struct
import zlib
import subprocess

# reference: virtualcdrom.hpp
CDTOC_SZ = 4096

def decode_audio(audio_file):
    print("  decoding '%s'" % os.path.basename(audio_file))
    r = subprocess.run(["./ffmpeg", "-i", audio_file, "-f", "s16le", "-"], capture_output=True, check=True)
    #print(r.stderr.decode())
    return r.stdout

def calc_crc(data):
    return "%08x" % zlib.crc32(data)

def nbytes_to_msf(nbytes):
    assert nbytes % 2352 == 0
    nsamples = nbytes // 4
    return (nsamples // 44100 // 60, nsamples // 44100 % 60, nsamples % 44100 // 588)

def msf_to_nbytes(msf):
    nsamples = msf[0] * 44100 * 60 + msf[1] * 44100 + msf[2] * 588
    return nsamples * 4

def msf_add(msf, nsamples):
    return nbytes_to_msf(msf_to_nbytes(msf) + nsamples * 4)

def msf_to_str(msf):
    return "%2d:%02d.%02d" % msf

def parse_cue(cue_file):
    cue = open(cue_file, "rb").read()
    cuetext = None
    for charset in ['utf_8', 'shift_jis', 'gbk', 'big5']:
        try:
            print("trying %s" % charset)
            cuetext = cue.decode(charset)
            break
        except UnicodeDecodeError:
            continue
    samples = None
    toc = []
    for line in cuetext.split('\n'):
        tokens = line.split()
        if len(tokens) >= 2 and tokens[0] == 'FILE':
            audio_file = os.path.join(os.path.dirname(cue_file), line.split('"')[1])
            print("loading disk image")
            assert samples == None
            samples = decode_audio(audio_file)
        if len(tokens) >= 3 and tokens[0] == 'INDEX' and tokens[1] == '01':
            toc.append(tuple(map(int, tokens[2].split(':'))))
    return toc, ['????????'] * len(toc), samples

def load_list(audio_files):
    samples_list = []
    crc_list = []
    toc = []
    offset = 0
    for i, audio_file in enumerate(audio_files):
        print("loading track %02d" % i)
        samples = decode_audio(audio_file)
        toc.append(nbytes_to_msf(offset))
        samples_list.append(samples)
        crc_list.append(calc_crc(samples))
        offset += len(samples)
    return toc, crc_list, b''.join(samples_list)

def dump_info(toc, crc, endmsf, endcrc):
    print('=' * 50)
    for i, msf in enumerate(toc):
        print("track %02d start %s sector %6d crc %s" % (i, msf_to_str(msf), msf_to_nbytes(msf) / 2352, crc[i]))
    print('-' * 50)
    print("total          %s sector %6d crc %s" % (msf_to_str(endmsf), msf_to_nbytes(endmsf) / 2352, endcrc))
    print('=' * 50)

def shift_toc(toc, endmsf, nsamples=44100*2):
    return list(map(lambda x: msf_add(x, nsamples), toc)), msf_add(endmsf, nsamples)

def encode_toc(toc, endmsf):
    binary_toc = []
    binary_toc.append(struct.pack('>H', 2+(3+len(toc))*11) + b'\x01\x01')
    binary_toc.append(b'\x01\x10\x00\xa0\x00\x00\x00\x00\x01\x00\x00')
    binary_toc.append(b'\x01\x10\x00\xa1\x00\x00\x00\x00' + bytes([len(toc)]) + b'\x00\x00')
    binary_toc.append(b'\x01\x10\x00\xa2\x00\x00\x00\x00' + bytes(endmsf))
    for i, msf in enumerate(toc):
        binary_toc.append(b'\x01\x10\x00' + bytes([i+1]) + b'\x00\x00\x00\x00' + bytes(msf))
    return b''.join(binary_toc)


infiles = sys.argv[1:]
if len(infiles) > 0:
    if os.path.splitext(infiles[0])[1].lower() == ".cue":
        toc, crc, samples = parse_cue(infiles[0])
    else:
        toc, crc, samples = load_list(infiles)
    
    endmsf = nbytes_to_msf(len(samples))
    dump_info(toc, crc, endmsf, calc_crc(samples))
    toc, endmsf = shift_toc(toc, endmsf)
    binary_toc = encode_toc(toc, endmsf)
else:
    binary_toc = b''
    samples = b''
    endmsf = (0,0,0)

gc.collect()

#print(binary_toc[:4].hex()+'\n'.join(map(lambda x: x.hex(), binary_toc[4:].split(b'\x01\x10\x00'))))
samples += b'\x00' * (msf_to_nbytes(endmsf) - len(samples))
print("sending %d bytes to kernel" % len(samples))
rawtoc = binary_toc + (CDTOC_SZ - len(binary_toc)) * b'\x00'
p = subprocess.Popen(["./client"], stdin=subprocess.PIPE)
p.stdin.write(rawtoc)
#input()
p.stdin.write(samples)
p.communicate()
