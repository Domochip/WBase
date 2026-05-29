import sys
import os
import glob
import gzip
import shutil

#Convert one file to header
#file is first GZipped then converted to header file (hex in PROGMEM)
def convert_file_to_cppheader(filename):
    with open(filename,'rb') as webfile:
        with gzip.open(filename+'.gz','wb',9) as intogzipfile:
            shutil.copyfileobj(webfile,intogzipfile)
    with open(filename+'.gz','rb') as gzfile:
        with open(filename+'.gz.h','w') as hfile:
            varname = os.path.basename(filename).replace(' ','').replace('.','').replace('-','')
            hfile.write('const PROGMEM char '+varname+'gz[] = {')
            byte=gzfile.read(1)
            first=True
            while len(byte):
                if not first:
                    hfile.write(',')
                if sys.version_info.major == 2:
                    hfile.write(hex(ord(byte)))
                else:
                    hfile.write(hex(byte[0]))
                first=False
                byte=gzfile.read(1)
            hfile.write('};')
    os.remove(filename+'.gz')

# Returns True if the .gz.h header is missing or its decompressed content differs from the source file
# The header is decoded from hex back to bytes and decompressed in memory
def is_convert_needed(filename):
    # check if gz.h file exists
    hfilename=filename+'.gz.h'
    if not os.path.exists(hfilename):
        return True
    # convert h file to gz file
    with open(hfilename) as hfile:
        # read the one line header file
        line=hfile.readline()
        # keep content between '{' and '}' and split hex values by ','
        hexvalues=line[line.find('{')+1:line.find('}')].split(',')
    # read file inside gz in memory
    decompressed=gzip.decompress(bytes(int(h,16) for h in hexvalues))
    # read web file
    with open(filename,'rb') as webfile:
        # return True if gz file content and web file are different
        return decompressed != webfile.read()

#Convert all Web Files in a folder
def convert_webfiles(pattern):
    for file in glob.glob(pattern):
        if is_convert_needed(file):
            print('Converting %s to header' % file)
            convert_file_to_cppheader(file)

print('--- pio_pre_webfiles_to_headers.py start ---')

convert_webfiles('./src/base/data/*.js')
convert_webfiles('./src/base/data/*.css')
convert_webfiles('./src/base/data/index.html')

print('--- pio_pre_webfiles_to_headers.py end ---')
print()