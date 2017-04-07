#include "minxml/mxml.h"
#include <assert.h>

#include "cdr_config.h"

CDR_SYSTEM_CONFIG g_cdr_systemconfig;


int cdr_get_item_value_str(mxml_node_t *root,char *name,char *sValue)
{
	mxml_node_t *node = NULL;
	if(root == NULL || name == NULL || sValue == NULL)
	{
		printf("%s param is null..\r\n",__FUNCTION__);
		return -1;
	}
	node = mxmlFindElement(root, root, name,NULL, NULL,MXML_DESCEND);
	if(node)
	{
		char *pText = NULL;
		pText = mxmlGetText(node,0);
		strcpy(sValue,pText);	
		return 0;
	}
	//printf("%s = %s \n",name,sValue);
	return -1;	
}
int cdr_get_item_value_int(mxml_node_t *root,char *name)
{	
	char chData[10];
	cdr_get_item_value_str(root,name,chData);
	return atoi(chData);	
}

int cdr_read_xml_to_cfg(void)
{
	FILE *fp;
	//int nRet = -1;
	mxml_node_t *tree = NULL;
	mxml_node_t *node;
	fp = fopen("/home/cdr_syscfg.xml", "r");
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);

	CDR_SYSTEM_CONFIG sSystemconfig;
	memset(&sSystemconfig,0x00,sizeof(CDR_SYSTEM_CONFIG));
	
	node = mxmlFindElement(tree, tree, "cdrSystemCfg",NULL, NULL,MXML_DESCEND);
	if(node == NULL)
	{
		printf("cdrSystemCfg node not found..\r\n");
		mxmlRelease(tree);		
		return -1;
	}
	sSystemconfig.volume = cdr_get_item_value_int(node,"volume");
	sSystemconfig.bootVoice = cdr_get_item_value_int(node,"bootVoice");


#if 0		
	sSystemconfig.photoWithVideo = cdr_get_item_value_int(node,"photoWithVideo");    
	sSystemconfig.collisionVideoUploadAuto = cdr_get_item_value_int(node,"collisionVideoUploadAuto");
	sSystemconfig.photoVideoUploadAuto = cdr_get_item_value_int(node,"photoVideoUploadAuto");

	cdr_get_item_value_str(node,"name",sSystemconfig.name);
	cdr_get_item_value_str(node,"password",sSystemconfig.password);


	sSystemconfig.volumeRecordingSensitivity = cdr_get_item_value_int(node,"volumeRecordingSensitivity");

	sSystemconfig.accelerationSensorSensitivity = cdr_get_item_value_int(node,"accelerationSensorSensitivity");
	sSystemconfig.graphicCorrect = cdr_get_item_value_int(node,"graphicCorrect");
	sSystemconfig.cdrPowerDown = cdr_get_item_value_int(node,"cdrPowerDown");
	sSystemconfig.eDog = cdr_get_item_value_int(node,"eDog");
	sSystemconfig.bluetooth = cdr_get_item_value_int(node,"bluetooth");
	sSystemconfig.fmFrequency = cdr_get_item_value_int(node,"fmFrequency");
	sSystemconfig.telecontroller = cdr_get_item_value_int(node,"telecontroller");

	cdr_get_item_value_str(node,"rtspLive",sSystemconfig.rtspLive);
	cdr_get_item_value_str(node,"rtspRecord",sSystemconfig.rtspRecord);
	
	sSystemconfig.graphicCorrect = cdr_get_item_value_int(node,"graphicCorrect");
	sSystemconfig.cdrPowerDown = cdr_get_item_value_int(node,"cdrPowerDown");

	node = mxmlFindElement(tree, tree, "cdrSystemInfomation",NULL, NULL,MXML_DESCEND);
	cdr_get_item_value_str(node,"cdrSoftwareVersion",sSystemconfig.sCdrSystemInfo.cdrSoftwareVersion);
	cdr_get_item_value_str(node,"cdrHardwareVersion",sSystemconfig.sCdrSystemInfo.cdrHardwareVersion);
	cdr_get_item_value_str(node,"cdrBuildTime",sSystemconfig.sCdrSystemInfo.cdrBuildTime);
	cdr_get_item_value_str(node,"cdrDeviceSN",sSystemconfig.sCdrSystemInfo.cdrDeviceSN);


	node = mxmlFindElement(tree, tree, "cdrSystemSDFilePath",NULL, NULL,MXML_DESCEND);
	cdr_get_item_value_str(node,"pathPhoto",sSystemconfig.sCdrFilePath.pathPhoto);
	cdr_get_item_value_str(node,"pathIndexPic",sSystemconfig.sCdrFilePath.pathIndexPic);
	cdr_get_item_value_str(node,"pathMp4",sSystemconfig.sCdrFilePath.pathMp4);
	cdr_get_item_value_str(node,"pathXml",sSystemconfig.sCdrFilePath.pathXml);
	cdr_get_item_value_str(node,"pathTrail",sSystemconfig.sCdrFilePath.pathTrail);
	cdr_get_item_value_str(node,"pathSystemLog",sSystemconfig.sCdrFilePath.pathSystemLog);

	
	node = mxmlFindElement(tree, tree, "videoRecord",NULL, NULL,MXML_DESCEND);

	sSystemconfig.sCdrVideoRecord.type = cdr_get_item_value_int(node,"type");
	sSystemconfig.sCdrVideoRecord.mode = cdr_get_item_value_int(node,"mode");
	sSystemconfig.sCdrVideoRecord.rotate = cdr_get_item_value_int(node,"rotate");


	node = mxmlFindElement(tree, tree, "osd",NULL, NULL,MXML_DESCEND);
	sSystemconfig.sCdrOsdCfg.logo = cdr_get_item_value_int(node,"logo");
	sSystemconfig.sCdrOsdCfg.time = cdr_get_item_value_int(node,"time");
	sSystemconfig.sCdrOsdCfg.speed = cdr_get_item_value_int(node,"speed");
	sSystemconfig.sCdrOsdCfg.engineSpeed = cdr_get_item_value_int(node,"engineSpeed");
	sSystemconfig.sCdrOsdCfg.position = cdr_get_item_value_int(node,"position");
	sSystemconfig.sCdrOsdCfg.personalizedSignature = cdr_get_item_value_int(node,"personalizedSignature");
	cdr_get_item_value_str(node,"psString",sSystemconfig.sCdrOsdCfg.psString);


	node = mxmlFindElement(tree, tree, "net",NULL, NULL,MXML_DESCEND);

	sSystemconfig.sCdrNetCfg.wifiMode = cdr_get_item_value_int(node,"wifiMode");
	cdr_get_item_value_str(node,"apSsid",sSystemconfig.sCdrNetCfg.apSsid);
	cdr_get_item_value_str(node,"apPasswd",sSystemconfig.sCdrNetCfg.apPasswd);
	cdr_get_item_value_str(node,"staSsid",sSystemconfig.sCdrNetCfg.staSsid);
	cdr_get_item_value_str(node,"staPasswd",sSystemconfig.sCdrNetCfg.staPasswd);
	sSystemconfig.sCdrNetCfg.dhcp = cdr_get_item_value_int(node,"dhcp");
	cdr_get_item_value_str(node,"ipAddr",sSystemconfig.sCdrNetCfg.ipAddr);
	cdr_get_item_value_str(node,"gateway",sSystemconfig.sCdrNetCfg.gateway);
	cdr_get_item_value_str(node,"netmask",sSystemconfig.sCdrNetCfg.netmask);
	cdr_get_item_value_str(node,"dns",sSystemconfig.sCdrNetCfg.dns);
#endif
	memcpy(&g_cdr_systemconfig,&sSystemconfig,sizeof(CDR_SYSTEM_CONFIG));
	mxmlRelease(tree);
	return 0;
	
}

int cdr_config_init(void)
{
	//Read all data to mem
	cdr_read_xml_to_cfg();

	return 0;
}


//param: reg GPIO_DIR value.
//nIndex:0-7 GPIOx_0 - GPIOx_7
//mode:0 input 1 output.
int cdr_set_gpio_mode(unsigned int reg,int nIndex,int mode)
{
	//只能设定GPIO_DIR值.
	if(reg & 0x0400 != 0x0400)
		return -1;

	unsigned int nValue;
	if(0 == HI_MPI_SYS_GetReg(reg,&nValue))
	{
		if(mode == 1)
		{
			nValue |= (0x01<<nIndex);
		}
		else
		{
			nValue &= ~(0x01<< nIndex);
		}
		HI_MPI_SYS_SetReg(reg,nValue);
	}
	return -1;	
}


