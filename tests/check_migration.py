"""Self-contained preservation/resource checks; no Orbitals checkout required."""
from pathlib import Path
import hashlib,json,re,subprocess,sys,tarfile,xml.etree.ElementTree as ET
root=Path(__file__).resolve().parents[1]
record=json.loads((root/'docs/migrations/001-orbitals-source.json').read_text())
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(root/'docs/migrations/001-orbitals-worktree.patch')==record['worktree_patch_sha256']
for path,expected in record['unchanged_contour'].items():
 # dev.3 relocates the two runtime panels byte-for-byte; normalize only UI paths.
 actual = root/path
 if path in ('res/Contour_panel_light.svg','res/Contour_panel_dark.svg'):
  assert not actual.exists(), 'Obsolete runtime panel: '+path
  actual = root/'res/contour'/Path(path).name
 if path == 'src/Contour.cpp':
  normalized = actual.read_text().replace('res/contour/Contour_panel_', 'res/Contour_panel_')
  assert hashlib.sha256(normalized.encode()).hexdigest()==expected, 'Contour behavior changed'
 else:
  assert sha(actual)==expected, 'Contour changed: '+path
for entry in record['files']:
 path=entry['destination']
 # Immutable DSP and non-panel assets must be exact source bytes.
 # dev.4 intentionally changes UI-only display sampling; DSP remains immutable.
 if path.startswith(('src/orbitals/dsp/','res/orbitals/')) and not path.endswith(('orbitals_panel_light.svg','orbitals_panel_dark.svg')):
  assert sha(root/path)==entry['source_sha256'], 'Imported behavior/artwork changed: '+path
baseline=record['audio_baseline'];assert sha(root/baseline['destination'])==baseline['sha256']
# dev.2/dev.3 update the two panels; all other imported assets remain exact.
for theme in ('light','dark'):
 panel=ET.parse(root/f'res/orbitals/orbitals_panel_{theme}.svg')
 labels=[e.get('aria-label') for e in panel.iter()]
 for label in ('L (MONO)','R','MIX / BAL'):assert labels.count(label)==1
 for label in ('AUDIO','X','Y','Z','MIX'):assert label not in labels
 assert not any(e.get('id') in ('circle777','circle797') for e in panel.iter())
# Regression: match actual L/R widget centers to the updated panel holes.
widget=(root/'src/orbitals/ui/OrbitalWidget.cpp').read_text()
xs=[float(x.rstrip('f')) for x in re.search(r'const float outputs\[\] = \{([^}]+)',widget)[1].split(',')]
y=float(re.search(r'Vec\(outputs\[i\], ([0-9.]+)f\)',widget)[1])
for theme in ('light','dark'):
 panel=ET.parse(root/f'res/orbitals/orbitals_panel_{theme}.svg')
 parents={child:parent for parent in panel.iter() for child in parent}
 holes={}
 for e in panel.iter():
  if e.get('id') not in ('circle672-9','circle787','circle716'):continue
  x,ypos=float(e.get('cx')),float(e.get('cy'))
  parent=parents.get(e)
  while parent is not None:
   transform=parent.get('transform','')
   if transform:
    match=re.fullmatch(r'translate\(([^,]+),([^\)]+)\)',transform)
    assert match, 'Unexpected output transform: '+transform
    dx,dy=map(float,match.groups());x+=dx;ypos+=dy
   parent=parents.get(parent)
  holes[e.get('id')]=(x,ypos)
 for i,key in enumerate(('circle672-9','circle787')):
  assert abs(xs[i]-holes[key][0])<1e-5 and abs(y-holes[key][1])<1e-5, (theme,key,'output misalignment')
 level=tuple(map(float,re.search(r'knob\(([0-9.]+)f, ([0-9.]+)f, Orbital::LEVEL\)',widget).groups()))
 assert all(abs(a-b)<1e-5 for a,b in zip(level,holes['circle716'])), (theme,'LEVEL misalignment')
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
print('PASS unchanged Contour implementation/artwork and Orbitals DSP/non-panel assets, provenance baseline, manifest, SVG resources and shared binary symbols')

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
