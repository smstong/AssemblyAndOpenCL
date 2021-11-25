/* kernel one */
const sampler_t sampler1 = CLK_NORMALIZED_COORDS_FALSE |
								CLK_ADDRESS_NONE |
								CLK_FILTER_LINEAR;

__kernel void bright(
	__read_only image2d_t inputImage,
	__write_only image2d_t outputImage)  
{
	int width = get_image_width(inputImage);
	int height = get_image_height(inputImage);

	const size_t gid = get_global_id(0);

	int x = gid / width;
	int y = gid % width;
	int2 coord = (int2)(x,y);

	uint4 in_v = read_imageui(inputImage, sampler1, coord);
	uint4 brighten_v = (uint4)(30,30,30,0);
	uint4 out_v = in_v + brighten_v;
	write_imageui(outputImage, coord, out_v);
}