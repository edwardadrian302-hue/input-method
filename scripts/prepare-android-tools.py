"""Download official portable Android tools into the project; no global install."""
import concurrent.futures,hashlib,json,pathlib,tarfile,urllib.request,zipfile
ROOT=pathlib.Path(__file__).resolve().parent.parent
SDK=ROOT/'.tools/android-sdk'
PACKAGES={
 'platforms/android-36':('platform-36_r02.zip','2c1a80dd4d9f7d0e6dd336ec603d9b5c55a6f576'),
 'build-tools/36.0.0':('build-tools_r36_windows.zip','f16ccffd34de8790dede813a6c7d8e2c11a27b50'),
 'ndk/28.2.13676358':('android-ndk-r28c-windows.zip','086bba43ff2f5eb0e387b15c8278bb4e0d89ba1d'),
 'cmake/3.22.1':('cmake-3.22.1-windows.zip','292778f32a7d5183e1c49c7897b870653f2d2c1b'),
 'cmdline-tools/19.0':('commandlinetools-win-13114758_latest.zip','54a582f3bf73e04253602f2d1c80bd5868aac115'),
}
def fetch(url,path):
 if not path.exists():
  tmp=path.with_suffix(path.suffix+'.part')
  if path.name=='android-ndk-r28c-windows.zip':
   total=748118221; chunk=4*1024*1024
   directory=ROOT/'.downloads/ndk-chunks';directory.mkdir(exist_ok=True)
   # Reuse only completed ranges from the interrupted single-connection download.
   prefix=tmp.stat().st_size if tmp.exists() else 0
   def piece(index):
    start=index*chunk;end=min(total,start+chunk)-1;size=end-start+1
    f=directory/str(index)
    if f.exists() and f.stat().st_size==size:return f
    if end<prefix:
     with tmp.open('rb') as stream:stream.seek(start);data=stream.read(size)
    else:
     for attempt in range(4):
      try:
       response=urllib.request.urlopen(urllib.request.Request(url,headers={'Range':f'bytes={start}-{end}'}),timeout=90)
       if response.status!=206 or response.headers.get('Content-Range')!=f'bytes {start}-{end}/{total}':raise RuntimeError('Invalid range response')
       data=response.read()
       if len(data)!=size:raise RuntimeError('Incomplete range')
       break
      except Exception:
       if attempt==3:raise
    f.write_bytes(data);return f
   with concurrent.futures.ThreadPoolExecutor(max_workers=16) as pool:
    pieces=list(pool.map(piece,range((total+chunk-1)//chunk)))
   with tmp.open('wb') as output:
    for f in pieces:output.write(f.read_bytes())
  else:urllib.request.urlretrieve(url,tmp)
  tmp.replace(path)
 return path

def package(entry):
 rel,(name,sha1)=entry
 archive=fetch('https://dl.google.com/android/repository/'+name,ROOT/'.downloads'/name)
 if hashlib.sha1(archive.read_bytes()).hexdigest()!=sha1:raise RuntimeError('Checksum mismatch: '+name)
 target=SDK/rel
 if not (target/'.chaoxi-ready').exists():
  with zipfile.ZipFile(archive) as z:
   for item in z.infolist():
    parts=pathlib.PurePosixPath(item.filename).parts
    if not name.startswith('cmake-'):parts=parts[1:]
    if not parts:continue
    if '..' in parts:raise RuntimeError('Unsafe archive')
    dest=target.joinpath(*parts)
    if item.is_dir():dest.mkdir(parents=True,exist_ok=True)
    else:dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(z.read(item))
  (target/'.chaoxi-ready').write_text(sha1+'\n')
 print('Ready:',rel,flush=True)
 return rel,{'url':'https://dl.google.com/android/repository/'+name,'sha1':sha1,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest()}
if __name__=='__main__':
 (ROOT/'.downloads').mkdir(exist_ok=True)
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:results=dict(pool.map(package,PACKAGES.items()))
 (ROOT/'android-tools.lock.json').write_text(json.dumps(results,indent=2)+'\n')
 gradle_url='https://services.gradle.org/distributions/gradle-8.13-bin.zip'
 gradle=fetch(gradle_url,ROOT/'.downloads/gradle-8.13-bin.zip')
 gradle_sha='20f1b1176237254a6fc204d8434196fa11a4cfb387567519c61556e8710aed78'
 if hashlib.sha256(gradle.read_bytes()).hexdigest()!=gradle_sha:raise RuntimeError('Gradle checksum mismatch')
 with zipfile.ZipFile(gradle) as z:
  for item in z.infolist():
   if '..' in pathlib.PurePosixPath(item.filename).parts:raise RuntimeError('Unsafe Gradle archive')
  z.extractall(ROOT/'.tools')
 results['gradle']={'url':gradle_url,'sha256':gradle_sha}
 (ROOT/'android-tools.lock.json').write_text(json.dumps(results,indent=2)+'\n')
 print('Tools ready. Use sdkmanager --licenses to review SDK license terms before building.',flush=True)
