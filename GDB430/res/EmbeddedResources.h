#pragma once

extern void *_binary_res_EraseDCO_bin_start, *_binary_res_EraseDCO_bin_end, *_binary_res_EraseDCO_bin_size;
extern void *_binary_res_EraseDCOX_bin_start, *_binary_res_EraseDCOX_bin_end, *_binary_res_EraseDCOX_bin_size;
extern void *_binary_res_EraseXv2_bin_start, *_binary_res_EraseXv2_bin_end, *_binary_res_EraseXv2_bin_size;
#ifdef __cplusplus
#ifndef CUSTOM_EMBEDDED_RESOURCE_CLASS
template <void **_Start, void **_End, void **_Size> class EmbeddedResource
{
public:
	void *data() { return _Start; }
	void *end() { return _End; }
	unsigned size()  { return (unsigned)_Size; }
};
#endif

namespace EmbeddedResources
{
	static EmbeddedResource<&_binary_res_EraseDCO_bin_start, &_binary_res_EraseDCO_bin_end, &_binary_res_EraseDCO_bin_size> res_EraseDCO_bin;
	static EmbeddedResource<&_binary_res_EraseDCOX_bin_start, &_binary_res_EraseDCOX_bin_end, &_binary_res_EraseDCOX_bin_size> res_EraseDCOX_bin;
	static EmbeddedResource<&_binary_res_EraseXv2_bin_start, &_binary_res_EraseXv2_bin_end, &_binary_res_EraseXv2_bin_size> res_EraseXv2_bin;
}
#endif
