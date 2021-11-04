#include "dm-linear.h"

void nova_get_multi_nvmm_info(struct super_block *sb, struct nova_sb_info *sbi)
{
    int i;
    int ret;
    long t_size = 0;
    long end_device_size;
    void *virt_addr = NULL;
    void *end_virt_addr = NULL;
    pfn_t __pfn_t;

    for(i=0;i<number_of_devices;i++)
    {
        multi_pm[i] = kzalloc(sizeof(dm_nvmm_info), GFP_ATOMIC);
		if(multi_pm[i] == NULL){
	    	nova_err(sb, "Couldn't allocate memory.\n");
	    	return -EINVAL;
		}

        multi_pm[i]->virt_addr = NULL;
    }
	/* TODO: Changing the hard coding to auto version */
    multi_pm[0]->host = "pmem0";
    multi_pm[1]->host = "pmem1";
	/*
    multi_pm[0]->host = "pmem0";
    multi_pm[1]->host = "pmem1";
    multi_pm[2]->host = "pmem2";
    multi_pm[3]->host = "pmem3";
    multi_pm[4]->host = "pmem4";
    multi_pm[5]->host = "pmem5";
    multi_pm[6]->host = "pmem6";
    multi_pm[7]->host = "pmem7";
	*/

  /*
	ret = bdev_dax_supported(sb->s_bdev, PAGE_SIZE);
	nova_dbg_verbose("%s: dax_supported = %d; bdev->super=0x%p",
			 __func__, ret, sb->s_bdev->bd_super);
	if (!ret) {
		nova_err(sb, "device does not support DAX\n");
		return -EINVAL;
	}
	*/
	sbi->s_bdev = sb->s_bdev;

    for(i=0; i<number_of_devices; i++)
    {
	    multi_pm[i]->dax_dev = fs_dax_get_by_host(multi_pm[i]->host);    
	    if (!(multi_pm[i]->dax_dev)) {
	    	nova_err(sb, "Couldn't retrieve DAX device.\n");
	    	return -EINVAL;
	    }

        multi_pm[i]->size = dax_direct_access(multi_pm[i]->dax_dev, 0, LONG_MAX/PAGE_SIZE,
				 &virt_addr, &__pfn_t) * PAGE_SIZE ;
	
        multi_pm[i]->virt_addr = virt_addr;
        multi_pm[i]->__pfn_t = __pfn_t;
	
        if (!virt_addr) {
	    	nova_err(sb, "ioremap of the nova image failed(1)\n");
	    	return -EINVAL;
        }

        if (multi_pm[i]->size <= 0) {
	    	nova_err(sb, "direct_access failed\n");
	    	return -EINVAL;
	    }

        t_size += multi_pm[i]->size;
        
	sbi->phys_addr = pfn_t_to_pfn(multi_pm[i]->__pfn_t) << PAGE_SHIFT;
	nova_dbg("Dev %d : dev %s, phys_addr 0x%llx, virt_addr 0x%lx, size %ld\n",
		i, multi_pm[i]->dax_dev,
		sbi->phys_addr, multi_pm[i]->virt_addr, multi_pm[i]->size);

    }
	    
    sbi->s_dax_dev = multi_pm[0]->dax_dev;
    sbi->virt_addr = multi_pm[0]->virt_addr;

	sbi->phys_addr = pfn_t_to_pfn(multi_pm[0]->__pfn_t) << PAGE_SHIFT;
	sbi->initsize = t_size;

    end_device_size = multi_pm[number_of_devices - 1]->size;
    end_virt_addr = multi_pm[number_of_devices -1]->virt_addr;    

	sbi->replica_reserved_inodes_addr = end_virt_addr + end_device_size -
			(sbi->tail_reserved_blocks << PAGE_SHIFT);
	sbi->replica_sb_addr = end_virt_addr + end_device_size - PAGE_SIZE;

	/*
	nova_dbg("1. %s: dev %s, phys_addr 0x%llx, virt_addr 0x%lx, size %ld\n",
		__func__, multi_pm[0]->dax_dev,
		sbi->phys_addr, (unsigned long)sbi->virt_addr, t_size);
    
    nova_dbg("2. %s: dev %s, phys_addr 0x%llx, virt_addr 0x%lx, size %ld\n",
		__func__, multi_pm[1]->dax_dev,	pfn_t_to_pfn(multi_pm[1]->__pfn_t) << PAGE_SHIFT,
        (unsigned long)multi_pm[1]->virt_addr, multi_pm[1]->size);
    */

	return 0;
}

u64 nova_get_addr_off_for_NUMA(struct nova_sb_info *sbi, void *addr)
{
    int i;
    void *addr_start = 0;   
    void *addr_end = 0;   
    void *total = 0;

    for(i=0; i<number_of_devices; i++)
    {
        addr_start = multi_pm[i]->virt_addr;
        addr_end = addr_start + multi_pm[i]->size;
        if(addr_start <= addr && addr < addr_end)
        {
            total += (addr - addr_start);
            return (u64)total;
        }        
        total += multi_pm[i]->size;
    }

    nova_dbg("%s: Fail to calculate offset\n", __func__);
    return -1;
}

void *nova_get_block_for_NUMA(struct super_block *sb, u64 block)
{
    int i, chk;
    int cpuid = nova_get_cpuid(sb);
    u64 range_begin = 0;
    u64 range_end = 0;

    for(i=0; i<number_of_devices; i++)
    {
        range_end += multi_pm[i]->size;

        if(range_begin <= block && block < range_end)
		{
	   // nova_dbg("%s: CPU: %d, DEV: %d, Block addr: %llx, Virt addr: %lx\n", __func__, cpuid, i, block, (multi_pm[i]->virt_addr + (block - range_begin)));
            return multi_pm[i]->virt_addr + (block - range_begin);
		}
        range_begin = range_end;
    }

    nova_dbg("%s: Fail to convert address\n", __func__);
    return -1;
}
