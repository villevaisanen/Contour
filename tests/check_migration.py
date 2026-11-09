"""Self-contained preservation/resource checks; no Orbitals checkout required."""
from pathlib import Path
import hashlib,json,re,subprocess,sys,tarfile,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
record=json.loads((root/'docs/migrations/001-orbitals-source.json').read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(root/'docs/migrations/001-orbitals-worktree.patch')==record['worktree_patch_sha256']
for path,expected in record['unchanged_contour'].items():
 assert sha(root/path)==expected, 'Contour changed: '+path
for entry in record['files']:
 path=entry['destination']
 # Immutable DSP, display calculation and assets must be exact source bytes.
 if path.startswith(('src/orbitals/dsp/','res/orbitals/')) or path.endswith('OrbitalDisplayModel.hpp'):
  assert sha(root/path)==entry['source_sha256'], 'Imported behavior/artwork changed: '+path
baseline=record['audio_baseline'];assert sha(root/baseline['destination'])==baseline['sha256']
manifest=json.loads((root/'plugin.json').read_text())
assert manifest['slug']=='FilthAssembly' and manifest['brand']=='Filth Assembly'
assert [m['slug'] for m in manifest['modules']]==['Contour','Orbitals']
assert manifest['license']=='GPL-3.0-only'
for p in (root/'src').rglob('*'):
 if p.suffix not in ('.cpp','.hpp'):continue
 for asset in re.findall(r'asset::plugin\(pluginInstance,\s*"([^"]+)"',p.read_text()):
  file=root/asset;assert file.is_file(),asset
  ET.parse(file)
# Both exported models must link into the single binary, one init and instance.
if (root/'plugin.so').exists():
 symbols=subprocess.check_output(['nm','-D','--defined-only',str(root/'plugin.so')],text=True)
 names=[line.split()[-1] for line in symbols.splitlines()]
 for name in ('init','pluginInstance','modelContour','modelOrbital'):assert names.count(name)==1,(name,names.count(name))
print('PASS unchanged Contour implementation/artwork and Orbitals DSP/display/assets, provenance baseline, manifest, SVG resources and shared binary symbols')

if '--package' in sys.argv:
 package=root/'dist'/('FilthAssembly-'+manifest['version']+'-lin-x64.vcvplugin')
 data=subprocess.check_output(['zstd','-d','-c',str(package)])
 import io
 with tarfile.open(fileobj=io.BytesIO(data)) as archive:
  packaged=json.load(archive.extractfile('FilthAssembly/plugin.json'))
  assert packaged==manifest
  for p in (root/'res').rglob('*'):
   if p.is_file():assert archive.extractfile('FilthAssembly/'+str(p.relative_to(root))).read()==p.read_bytes()
  assert archive.extractfile('FilthAssembly/licence.txt') and archive.extractfile('FilthAssembly/LICENSES.md')
 print('PASS one combined package, exact manifest, all runtime assets and licensing')
