import urllib.request,pathlib,zipfile,hashlib,json,tarfile
root=pathlib.Path(__file__).resolve().parent.parent;dest=root/'vendor/android-deps';dest.mkdir(parents=True,exist_ok=True)
deps={
'yaml-cpp':('jbeder/yaml-cpp','56e3bb550c91fd7005566f19c079cb7a503223cf'),
'leveldb':('google/leveldb','bb74ef739973a70ca9f0d90788a1da89b788c0ad'),
'opencc':('BYVoid/OpenCC','025f371dc76b598d77384fbdab90c937471844d8')}
lock={}
for name,(repo,commit) in deps.items():
 archive=root/'.downloads'/('android-'+name+'.zip')
 url='https://codeload.github.com/'+repo+'/zip/'+commit
 if not archive.exists():urllib.request.urlretrieve(url,archive)
 sha=hashlib.sha256(archive.read_bytes()).hexdigest()
 expected=json.loads((root/'android-dependencies.lock.json').read_text()).get(name,{}) if (root/'android-dependencies.lock.json').exists() else {}
 if expected and (expected['commit']!=commit or expected['sha256']!=sha):raise RuntimeError('Dependency checksum mismatch: '+name)
 lock[name]={'url':url,'commit':commit,'sha256':sha}
 target=dest/name
 if not target.exists():
  target.mkdir()
  with zipfile.ZipFile(archive) as z:
   for info in z.infolist():
    parts=pathlib.PurePosixPath(info.filename).parts[1:]
    if not parts:continue
    if '..' in parts:raise RuntimeError('Unsafe archive')
    f=target.joinpath(*parts)
    if info.is_dir():f.mkdir(parents=True,exist_ok=True)
    else:f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(z.read(info))
 print(name,commit,flush=True)
boost_url='https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.bz2'
boost=root/'.downloads/boost_1_89_0.tar.bz2'
if not boost.exists():urllib.request.urlretrieve(boost_url,boost)
boost_sha='85a33fa22621b4f314f8e85e1a5e2a9363d22e4f4992925d4bb3bc631b5a0c7a'
if hashlib.sha256(boost.read_bytes()).hexdigest()!=boost_sha:raise RuntimeError('Boost checksum mismatch')
with tarfile.open(boost) as tar:
 for item in tar:
  if item.name.startswith('boost_1_89_0/boost/') or item.name=='boost_1_89_0/LICENSE_1_0.txt':
   if '..' in pathlib.PurePosixPath(item.name).parts:raise RuntimeError('Unsafe archive')
   if item.isfile() or item.isdir():tar.extract(item,dest)
lock['boost']={'url':boost_url,'version':'1.89.0','sha256':boost_sha}
license_url='https://raw.githubusercontent.com/Tencent/rapidjson/v1.1.0/license.txt'
license_file=root/'.downloads/rapidjson-license.txt'
if not license_file.exists():urllib.request.urlretrieve(license_url,license_file)
license_sha='a140e5d46fe734a1c78f1a3c3ef207871dd75648be71fdda8e309b23ab8b1f32'
if hashlib.sha256(license_file.read_bytes()).hexdigest()!=license_sha:raise RuntimeError('RapidJSON license checksum mismatch')
lock['rapidjson-license']={'url':license_url,'sha256':license_sha}
(root/'android-dependencies.lock.json').write_text(json.dumps(lock,indent=2)+'\n')
