# Analytical foundation

Imported from the Orbitals 2.1.2 worktree. Only mathematical Markdown notation
has been converted to Obsidian-compatible LaTeX; the engine is unchanged.

In atomic units $Z=a_0=1$, let

$$
\begin{aligned}
r&=\sqrt{x^2+y^2+z^2}, & \rho&=\frac{2r}{n}, & k&=n-l-1,\\
\psi&=\sqrt{\left(\frac{2}{n}\right)^3\frac{k!}{2n(n+l)!}}\,
 e^{-r/n}\left(\frac{2}{n}\right)^l L_k^{2l+1}(\rho)S_l(x,y,z),\\
S_l&=r^lY_l^{\mathrm{real}}.
\end{aligned}
$$

Combining powers of $r$ with the real spherical harmonics removes all divisions
by $r$, avoiding spherical angles and the origin singularity. Radial and angular
normalizations are retained. The double-precision evaluator returns signed
$\psi$, never $|\psi|^2$.

The exact Laguerre polynomials required here are:

$$
\begin{aligned}
L_0^a(\rho)&=1,\\
L_1^a(\rho)&=a+1-\rho,\\
L_2^a(\rho)&=\frac{\rho^2-2(a+2)\rho+(a+2)(a+1)}{2}.
\end{aligned}
$$

Real solid harmonics use positive Cartesian coefficients as the global phase convention:

| Harmonic | $S_l$ |
| --- | --- |
| $s$ | $1/\sqrt{4\pi}$ |
| $p_x,p_y,p_z$ | $\sqrt{3/(4\pi)}\,(x,y,z)$ respectively |
| $d_{xy},d_{xz},d_{yz}$ | $\sqrt{15/(4\pi)}\,(xy,xz,yz)$ respectively |
| $d_{x^2-y^2}$ | $\sqrt{15/(16\pi)}\,(x^2-y^2)$ |
| $d_{z^2}$ | $\sqrt{5/(16\pi)}\,(2z^2-x^2-y^2)$ |

IDs 0, 1, 5 use $l=0$; IDs 2–4 and 6–8 use $l=1$; IDs 9–13 use $l=2$.
The principal quantum numbers are $n=1,2,3$ respectively by orbital shell.
No orbital values or audio waveforms are cached.

Independent Cartesian reference formulas:

$$
\begin{aligned}
\psi_{1s}&=\frac{e^{-r}}{\sqrt{\pi}},\\
\psi_{2s}&=\frac{(2-r)e^{-r/2}}{4\sqrt{2\pi}},\\
\psi_{2p_i}&=\frac{i\,e^{-r/2}}{4\sqrt{2\pi}},\qquad i\in\{x,y,z\},\\
\psi_{3s}&=\frac{(27-18r+2r^2)e^{-r/3}}{81\sqrt{3\pi}},\\
\psi_{3p_i}&=\frac{\sqrt{2}\,i(6-r)e^{-r/3}}{81\sqrt{\pi}},\\
D&=\frac{2e^{-r/3}}{81\sqrt{2\pi}}.
\end{aligned}
$$

The five $3d$ states are $Dxy$, $Dxz$, $Dyz$, $D(x^2-y^2)/2$ and
$D(2z^2-x^2-y^2)/(2\sqrt{3})$.

## Superposition and transitions

For different states, $w_A=\cos(\pi m/2)$ and $w_B=\sin(\pi m/2)$.
Orthogonality ensures unit spatial norm. If $A=B$, the target coefficient is
exactly 1, equivalent to dividing the uncorrected sum by
$\cos(\pi m/2)+\sin(\pi m/2)$. One common audio gain follows summation.

Selection/MIX changes drive a nonnegative 14-element coefficient vector through
a 5 ms one-pole transition. The transient vector is not renormalized: its norm
may dip during a selection fade, avoiding amplification of weak signals.
Once settled, the requested normalized superposition is recovered. This is an
audio transition, not physical time evolution.

## Trajectory

With $s=\mathrm{SHAPE}$, $\theta=2\pi\,\mathrm{phase}$ and $R=\mathrm{RADIUS}$:

$$
\begin{aligned}
x&=R\sin\theta,\\
y&=R(0.35+0.65s)\sin(2\theta+\pi s),\\
z&=R(1-0.7s)\sin(3\theta+\pi s/2).
\end{aligned}
$$

Rotate $y,z$ around $X$ by $\pi\,\mathrm{TILT}$, then add OFFSET to $x$.
One-cycle closure is preserved at fixed shape. Double/triple-angle identities
reuse $\sin\theta$ and $\cos\theta$. Field sampling uses inverse Z rotation
$R_z(-2\pi\,\mathrm{spinPhase})$; spinPhase integrates SPIN independently.
Nonzero spin or varying parameters can prevent audio repetition after one cycle.

## Audio

$$
f=\min\!\left(20000,\;0.45f_s,\;261.6255653\times
2^{\mathrm{pitch}+\mathrm{V/OCT}+\mathrm{FM}}\right).
$$

Inputs are bounded and the total exponent is limited to $[-16,16]$.
Negative exponential FM lowers frequency; through-zero linear FM is not implemented.

Four analytical samples per host sample feed a 95-tap unity-DC-gain Blackman-windowed
sinc FIR, cutoff 0.105 cycles/internal sample (0.42 cycles/host sample).
Coefficients are constructed once and the decimator uses fixed arrays. Then:

$$
y[n]=x[n]-x[n-1]+e^{-2\pi\,5/f_s}y[n-1].
$$

LEVEL and a $\pm5\,\mathrm{V}$ safety clamp follow. Sample-rate changes update the
DC pole and smoothing without resetting phase. Optional hard sync resets phase
while preserving filter history.

## Rack interface in 2.1.2 and the combined plugin

Orbitals exposes six CV inputs. FM and Sync are not connected to its engine and
remain zero. Their optional engine behavior above is preserved for regression
tests, not exposed as new controls in this migration.

## References

- [Hydrogen radial solutions and real spherical harmonics](https://www.physchem.no/physchem_book/chapters-11-hydrogen.html)
- [Rack plugin API](https://vcvrack.com/manual/PluginGuide)
- [Rack installation and command-line options](https://vcvrack.com/manual/Installing)
