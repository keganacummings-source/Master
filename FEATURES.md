# INXOMNIA effect rack

The rack uses 200 unique effect IDs and display names based on the supplied INSOMNIA.html FX_DEFS list. No native IDs or display names are duplicated. Each effect has an enable control and an amount control, with host automation and saved state. Existing host parameter IDs (`fx0`–`fx199` and `amt0`–`amt199`) are preserved.

The compact category chips and search field filter the rack without changing stable effect indices. Favorites are stored in APVTS state by stable feature ID, appear first in results, and have a dedicated favorites filter. Favorite cards keep a visible `FAV *` marker and gold outline even when unselected. Only the filtered cards are instantiated. `RANDOM FX` chooses 1–10 distinct effects and assigns the stable per-effect values; `RANDOM VALUES` chooses 1–10 with genuinely random amounts; `XTRMRND` chooses 10–20 with random amounts. All Off disables the entire rack.

The FX Builder is gated by an authenticated DreamShare `plugin_capabilities` response with `fxBuilder: true`. Its two-column workspace separates a virtualized category-filtered module list from the ordered chain. Choosing a module sets the new step's amount; choosing a chain row changes that step's amount. Reorder/remove and preset controls are grouped with the chain. Every current effect has one functional APVTS amount parameter; no extra controls are added without a corresponding DSP parameter. Custom preset IDs, effect IDs, amounts, labels, favorite IDs, and active chain order are stored inside APVTS plugin state, not in external files; host `fxN`/`amtN` parameters and automation IDs are unchanged. Worker requests run on a background thread and marshal replies to the UI thread. Passwords and auth tokens are never serialized into project state; Windows stores the short-lived token in the OS Credential Manager, while unsupported platforms keep it in memory only. The `session` response's optional refreshed token is saved back to the OS store.

All 16 themes in the Worker catalog are listed for authenticated users. The native UI applies relevant color values from the theme pack's `vars` object, and ignores web-only file/scene/veil fields. The GOONR and Trippah themes use restrained native animation while the editor is visible.

Every slot is explicitly assigned a native DSP recipe; there is no substring classifier or hash-derived routing. The recipes reuse stable processing primitives (filters, saturation, dynamics, modulated delay, all-pass phasing, feedback delay, reverb combs, stereo imaging, and rhythmic gating) with deliberately varied timing, tone, depth, and response. Enable/amount ramps, bounded feedback, DC blocking, finite-value guards, and a linked sample-peak limiter improve behavior with arbitrary or stacked inputs. The limiter is not a true-peak limiter or loudness mastering processor.

The CTest target `DreamMasterDSPTests` renders all 200 effects and checks unique IDs/names, non-silent and pairwise-distinct outputs, selected modulation behavior, mono/stereo handling, finite output, and safety bounds at 44.1 and 96 kHz. Automated DSP checks do not substitute for listening in a DAW.

## Effect names

001. Drive
002. Doubler / Chorus
003. Delay
004. Reverb
005. Stereo Width
006. High-Pass
007. Low-Pass
008. Tremolo / Chop
009. Auto-Pan
010. Flanger
011. Phaser
012. Bitcrush / Lofi
013. Vibrato
014. Slapback
015. Telephone
016. Tape / Wow
017. Stutter / Gate
018. Formant Body
019. Short Plate
020. Interval Comb
021. Mid Glue
022. Warmth
023. Predelay
024. Fade / Shape
025. Grain Scramble
026. Melt
027. Worm / Wriggle
028. Rot
029. Infection Echo
030. Crawl Trail
031. Star Air
032. Devil Grit
033. Moon Haze
034. Tower Bite
035. Vortex Orbit
036. Seance Choir
037. Ripple Spray
038. Halo Bloom
039. Euclid Pulse
040. Fog Bank
041. Needle Wow
042. Draw Ink
043. Loud / Limiter
044. Soft Clip
045. Tilt EQ
046. Bass Shelf
047. Air Shelf
048. Presence
049. Mud Cut
050. Box Cut
051. Hum Notch
052. Haas Width
053. Mono Bass
054. Short Room
055. Hall
056. Exciter
057. Warmth 2
058. Satin Roll
059. Polish Pump
060. Shimmer
061. Bus Chorus
062. Focus
063. Trim
064. Balance
065. Sheen
066. Soft Sat
067. Edge
068. Bloom
069. Swirl Filter
070. Ping
071. Drift
072. Snap
073. Body
074. Air Lift
075. Sub Boost
076. Notch Sweep
077. Ring Light
078. Deep Phase
079. Long Echo
080. Freeze
081. Vinyl
082. Radio
083. Underwater
084. Glass
085. Pulse Amp
086. Swirl Pan
087. Cream
088. Fog Bank 2
089. Glue II
090. Ceiling
091. Bus Drive
092. Air Max
093. Low End
094. Mid Carve
095. Widen II
096. Mono 80
097. Polish LP
098. Polish HP
099. Sparkle
100. Velvet
101. Punch
102. Silk
103. Mass
104. Polish Verb
105. Image
106. Final Loud
107. Curve
108. Clean
109. Stage
110. Wire
111. Parallel Glue
112. Shelf Master
113. Final Trim
114. Sub Lift
115. Presence Peak
116. Mud Notch
117. Box Notch
118. Night Air
119. Sleep Gate
120. Red Room
121. Acid Bite
122. Ghost Delay
123. Bone EQ
124. Marrow
125. Cinder
126. Ash Shelf
127. Cobalt
128. Ember
129. Lilac Halo
130. Pine Room
131. Ink Drip
132. Needle Flutter
133. Halo II
134. Ripple II
135. Orbit
136. Choir Bed
137. Pluck Slap
138. Drum Glue
139. Bass Focus
140. Hat Air
141. Snap Sat
142. Room Tap
143. Dark Hall
144. Bright Plate
145. Slow Flange
146. Fast Trem
147. Wide Chorus
148. Tight Comp
149. Open Comp
150. De-ess
151. Low Mono
152. Side Air
153. Crush II
154. Wow II
155. Flutter
156. 16th Gate
157. Shimmer Bloom
158. Low Ring Mod
159. High Ring Mod
160. Resonant
161. Smear
162. Dust
163. Tape II
164. Limiter II
165. Clip II
166. Air II
167. Width III
168. Hall II
169. Room II
170. Delay II
171. Phaser II
172. Chorus II
173. Vibrato II
174. Pan II
175. Trem II
176. Low-Pass II
177. High-Pass II
178. Drive II
179. Grit II
180. Haze II
181. Rot II
182. Melt II
183. Worm II
184. Infection II
185. Crawl II
186. Star II
187. Tower II
188. Vortex II
189. Seance II
190. Fog II
191. Ink II
192. Needle III
193. Halo III
194. Pulse II
195. Ripple III
196. Orbit II
197. Choir II
198. Plate II
199. Comb II
200. Formant II
