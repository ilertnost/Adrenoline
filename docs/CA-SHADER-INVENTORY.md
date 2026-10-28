# CoreAnimation shader inventory

Artifact: `System/Library/Frameworks/QuartzCore.framework/default.metallib`
Image: iOS 16.7.16 `20H392`, iPhone 8 (`D20AP`, `iPhone10,1`)

This file is **the compositor's entire GPU shader set** on the device we are
targeting. It is data extracted from the firmware, not code we reuse; it is
the workload specification Adrenoline has to be able to run.

| metric | value |
|---|---:|
| functions | 141 |
| vertex | 39 |
| fragment | 95 |
| kernel | 7 |
| AIR payload | 829968 bytes |
| container | 1014944 bytes |

## What the list says about scope

Read the names and the 3-30 KB AIR sizes and the shape of the problem falls out:

- **no tessellation, no mesh shaders, no ray tracing, no argument buffers**:
  nothing in this library needs anything past Metal 2 / Apple4.
- the expensive parts are **blur** (`downsample_*`, `narrow_blur_*`,
  `variable_blur_*`, `single_pass_blur_3`) and **path rasterisation**
  (`path_cubic`, `path_rect`, `path_stroke_*`, join variants).
- color management is a first-class citizen: `p3_clamp`, `edr_scale`,
  `luma_boost`, `apply_apl_dimming`, `display_overdrive_*`.
- `home_affordance_base_lpf` is the home indicator bar, i.e. exactly the thing
  that has to be on screen for the UI to look alive.
- two parallel sets exist, `_lpf` and `_lph` (plus `fixed_*_lph_*`), so the
  compositor picks between at least two quality/power variants per effect.

A frozen NIR/IR3 path only has to cover this set. That is a finite, countable
target instead of "all of Metal".

## How to reproduce

```sh
ipsw extract --files --pattern '.*\.metallib$' -o fs iPhone_4.7_P3_16.7.16_20H392_Restore.ipsw
tools/mtlbinfo.py fs/.../System/Library/Frameworks/QuartzCore.framework/default.metallib
```

## Full inventory

### core (4)

| kind | AIR bytes | name |
|---|---:|---|
| vertex | 3504 | `std_vert0_lpf` |
| vertex | 3520 | `std_vert1_lpf` |
| vertex | 3504 | `std_vert0_lph` |
| vertex | 3536 | `std_vert1_lph` |

### other (40)

| kind | AIR bytes | name |
|---|---:|---|
| vertex | 3632 | `upsample_vert_lpf` |
| vertex | 3504 | `asg_vert_lpf` |
| vertex | 4400 | `stroke_line_vert_lpf` |
| fragment | 4416 | `upsample_frag_lpf` |
| fragment | 4176 | `downsample_4_frag_lpf` |
| fragment | 4832 | `downsample_8_frag_lpf` |
| fragment | 4128 | `downsample_6_frag_lpf` |
| fragment | 7008 | `asg_frag_lpf` |
| fragment | 5312 | `asg_75_frag_lpf` |
| fragment | 5504 | `asg_50_frag_lpf` |
| fragment | 4240 | `alpha_threshold_lpf` |
| fragment | 4048 | `alpha_linear_premultiply_lpf` |
| fragment | 4704 | `home_affordance_base_lpf` |
| fragment | 2848 | `attachment_clear_frag_lpf` |
| fragment | 3392 | `inplace_copy_lpf` |
| fragment | 3360 | `stroke_line_frag_lpf` |
| vertex | 8304 | `fixed_vert_lph_spc` |
| vertex | 9616 | `fixed_vert_lph_gen` |
| vertex | 8288 | `fixed_vert_lpf_spc` |
| vertex | 9712 | `fixed_vert_lpf_gen` |
| fragment | 103728 | `fixed_frag_lph_cph` |
| fragment | 104384 | `fixed_frag_lph_cpf` |
| vertex | 3648 | `upsample_vert_lph` |
| vertex | 3520 | `asg_vert_lph` |
| vertex | 4496 | `stroke_line_vert_lph` |
| vertex | 2896 | `read_surf_vert` |
| fragment | 4512 | `upsample_frag_lph` |
| fragment | 4192 | `downsample_4_frag_lph` |
| fragment | 4832 | `downsample_8_frag_lph` |
| fragment | 4128 | `downsample_6_frag_lph` |
| fragment | 7024 | `asg_frag_lph` |
| fragment | 5392 | `asg_75_frag_lph` |
| fragment | 5456 | `asg_50_frag_lph` |
| fragment | 4240 | `alpha_threshold_lph` |
| fragment | 3936 | `alpha_linear_premultiply_lph` |
| fragment | 4688 | `home_affordance_base_lph` |
| fragment | 2848 | `attachment_clear_frag_lph` |
| fragment | 3408 | `inplace_copy_lph` |
| fragment | 3376 | `stroke_line_frag_lph` |
| fragment | 3216 | `read_surf_frag` |

### blur (26)

| kind | AIR bytes | name |
|---|---:|---|
| vertex | 3520 | `downsample_blur_vert_lpf` |
| vertex | 3552 | `variable_blur_vert_lpf` |
| fragment | 4160 | `downsample_blur_2_frag_lpf` |
| fragment | 4464 | `downsample_blur_4_frag_lpf` |
| fragment | 4256 | `narrow_blur_7_frag_lpf` |
| fragment | 4352 | `narrow_blur_11_frag_lpf` |
| fragment | 4448 | `narrow_blur_15_frag_lpf` |
| fragment | 4544 | `narrow_blur_19_frag_lpf` |
| fragment | 4656 | `narrow_blur_23_frag_lpf` |
| fragment | 4752 | `narrow_blur_27_frag_lpf` |
| fragment | 4272 | `single_pass_blur_3_lpf` |
| fragment | 4160 | `variable_blur_downsample_frag_lpf` |
| fragment | 7200 | `variable_blur_frag_lpf` |
| vertex | 3536 | `downsample_blur_vert_lph` |
| vertex | 3552 | `variable_blur_vert_lph` |
| fragment | 4176 | `downsample_blur_2_frag_lph` |
| fragment | 4464 | `downsample_blur_4_frag_lph` |
| fragment | 4256 | `narrow_blur_7_frag_lph` |
| fragment | 4352 | `narrow_blur_11_frag_lph` |
| fragment | 4448 | `narrow_blur_15_frag_lph` |
| fragment | 4560 | `narrow_blur_19_frag_lph` |
| fragment | 4656 | `narrow_blur_23_frag_lph` |
| fragment | 4752 | `narrow_blur_27_frag_lph` |
| fragment | 4272 | `single_pass_blur_3_lph` |
| fragment | 4176 | `variable_blur_downsample_frag_lph` |
| fragment | 7312 | `variable_blur_frag_lph` |

### path/stroke (36)

| kind | AIR bytes | name |
|---|---:|---|
| vertex | 5648 | `path_cubic_vert_0_lpf` |
| vertex | 5648 | `path_cubic_vert_1_lpf` |
| vertex | 4384 | `path_rect_vert_lpf` |
| vertex | 4336 | `path_stroke_line_vert_lpf` |
| vertex | 5536 | `path_stroke_curve_vert_0_lpf` |
| vertex | 5552 | `path_stroke_curve_vert_1_lpf` |
| vertex | 5264 | `path_stroke_miter_join_vert_lpf` |
| vertex | 4160 | `path_stroke_round_join_vert_lpf` |
| vertex | 3456 | `path_blit_vert_lpf` |
| fragment | 3648 | `path_cubic_frag_lpf` |
| fragment | 3312 | `path_rect_frag_lpf` |
| fragment | 3264 | `path_stroke_line_frag_lpf` |
| fragment | 3696 | `path_stroke_curve_frag_lpf` |
| fragment | 3328 | `path_stroke_miter_join_frag_lpf` |
| fragment | 3856 | `path_stroke_round_join_frag_lpf` |
| fragment | 2800 | `path_clear_frag_lpf` |
| fragment | 3424 | `path_blit_frag_lpf` |
| fragment | 3488 | `path_blit_frag_eo_lpf` |
| vertex | 5648 | `path_cubic_vert_0_lph` |
| vertex | 5648 | `path_cubic_vert_1_lph` |
| vertex | 4384 | `path_rect_vert_lph` |
| vertex | 4336 | `path_stroke_line_vert_lph` |
| vertex | 5536 | `path_stroke_curve_vert_0_lph` |
| vertex | 5552 | `path_stroke_curve_vert_1_lph` |
| vertex | 5264 | `path_stroke_miter_join_vert_lph` |
| vertex | 4160 | `path_stroke_round_join_vert_lph` |
| vertex | 3456 | `path_blit_vert_lph` |
| fragment | 3648 | `path_cubic_frag_lph` |
| fragment | 3312 | `path_rect_frag_lph` |
| fragment | 3264 | `path_stroke_line_frag_lph` |
| fragment | 3696 | `path_stroke_curve_frag_lph` |
| fragment | 3328 | `path_stroke_miter_join_frag_lph` |
| fragment | 3856 | `path_stroke_round_join_frag_lph` |
| fragment | 2800 | `path_clear_frag_lph` |
| fragment | 3424 | `path_blit_frag_lph` |
| fragment | 3472 | `path_blit_frag_eo_lph` |

### blit/clear (4)

| kind | AIR bytes | name |
|---|---:|---|
| vertex | 4096 | `buffer_image_blit_vert_lpf` |
| fragment | 3920 | `buffer_image_blit_frag_lpf` |
| vertex | 4128 | `buffer_image_blit_vert_lph` |
| fragment | 3936 | `buffer_image_blit_frag_lph` |

### gradient (8)

| kind | AIR bytes | name |
|---|---:|---|
| fragment | 3728 | `axial_shading_frag_lpf` |
| fragment | 4560 | `radial_shading_frag_lpf` |
| fragment | 5360 | `radial_complex_shading_frag_lpf` |
| fragment | 4400 | `conic_shading_frag_lpf` |
| fragment | 3744 | `axial_shading_frag_lph` |
| fragment | 4560 | `radial_shading_frag_lph` |
| fragment | 5376 | `radial_complex_shading_frag_lph` |
| fragment | 4416 | `conic_shading_frag_lph` |

### color/display (19)

| kind | AIR bytes | name |
|---|---:|---|
| fragment | 4368 | `p3_clamp_lpf` |
| fragment | 4736 | `edr_scale_lpf` |
| fragment | 4416 | `harmony_lpf` |
| fragment | 4112 | `display_overdrive_lut_gen_lpf` |
| fragment | 6128 | `display_overdrive_linearize_lpf` |
| fragment | 6000 | `display_overdrive_srgb_sampler_lpf` |
| fragment | 4160 | `luma_boost_lpf` |
| fragment | 4080 | `apply_apl_dimming_lpf` |
| kernel | 5360 | `compute_apl` |
| kernel | 5728 | `compute_apl_fast` |
| kernel | 5152 | `sum_apl_and_compute_dimming_factor` |
| fragment | 4208 | `p3_clamp_lph` |
| fragment | 4624 | `edr_scale_lph` |
| fragment | 4272 | `harmony_lph` |
| fragment | 4128 | `display_overdrive_lut_gen_lph` |
| fragment | 5984 | `display_overdrive_linearize_lph` |
| fragment | 5872 | `display_overdrive_srgb_sampler_lph` |
| fragment | 4176 | `luma_boost_lph` |
| fragment | 4112 | `apply_apl_dimming_lph` |

### tile (4)

| kind | AIR bytes | name |
|---|---:|---|
| kernel | 3792 | `tile_downsample_1` |
| kernel | 3872 | `tile_downsample_2` |
| kernel | 4096 | `tile_downsample_4` |
| kernel | 4528 | `tile_downsample_8` |

