cd osdrv/
tar cvzf app.tar.gz app/
cp -f app.tar.gz pub/rootfs_uclibc/home/ 
cd ..

osdrv/pub/bin/pc/mkfs.jffs2 -d osdrv/pub/rootfs_uclibc -l -e 0x10000 -o osdrv/pub/rootfs_uclibc_64k.jffs2
cd osdrv/pub
mkimage -A arm -T filesystem -C none -n hirootfs -d rootfs_uclibc_64k.jffs2 rootfs
 cp rootfs /nfs/sample/hisi3516v200/cdrfcimage/
cd -
