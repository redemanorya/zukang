
为了避免麻烦，添加自定义音效前，请检视一下需要添加的音效是否符合如下特征：
1. 音效支持最多2个输入源，每个输入源可以是mono或者stereo，或者mono和stereo同时支持。
2. 音效支持最多1个输出源，可以是mono或者stereo，或者mono和stereo同时支持。
3. 单双声道模式必须统一，比如某个配置下，2个输入，1个输出都是单声道，或都是双声道。不允许1个输入单声道，1个双声道，然后输出又是单声道，这种无序组合。
4. 参数的精度必须是有符号16bit，可以支持比如Q1.15这种定点数，或者固定的小数位数，比如10.23这种，上位机可以配合做小数位的显示和数制的比例调整。
5. 基于第4点，参数不支持float，double等类型。
6. 音效context memory中如果存在指针（指向memory中的某个区域），那么每次apply时需要重新刷新指针值。
原因在于内部框架会memory move整块context memory，移动后指针数据即失效。
7. apply函数中，数据的input，output都以int16_t* 指针形式传递，如果碰到音效原生apply函数使用非int16_t*形式，需要自行转换。
比如数据使用float32，可以参考nds32_convert_q15_f32，nds32_convert_f32_q15进行输入输出的转换。


生成自定义音效信息的一般过程：
1. 将标准音效信息roboeffect_library_info_v2.xx.xx.bin文件拷贝到此目录下。
2. 在此新建一个目录，包含自定义音效的所有必要文件，如UserLibsData.xlsx，接口实现源文件，头文件等；目录名请使用英文，按C语言变量规则。
3. 添加并修改UserLibsData.xlsx，添加参数信息。注意修改UserLibsData.xlsx的sheet名称，请保证新建的目录名和sheet名保持一致。
典型的目录结构如下：
.
├─ readme.txt
├─ roboeffect_library_info_v2.34.0.bin --> 必须放入这个文件后，执行“third_party_effects_data_gen.exe”
├─ simple_gain
│   ├─ simple_gain.c
│   ├─ simple_gain.h
│   ├─ simple_gain_default_params.txt
│   └─ UserLibsData.xlsx
├─ third_party_effects_data_gen.exe
└─ user_defined_effect_api.h

4. 双击third_party_effects_data_gen.exe，会自动生成user_defined_effect_api.h，以及每个子目录里的xxxx_default_params.txt
5. xxxx_default_params.txt包含缺省参数，以及4个接口函数的实现模板，可以自行拷贝到源文件进行填充代码。
6. 将roboeffect_library_info_v2.xx.xx_3rdparty.bin拷贝到调音工具的\ACPWorkbench_V3.X.X\roboeffect_library_info_set\目录下，可以做离线调音使用。

可能会碰到的问题：
在使用某些非标准的编辑器（如WPS）编辑UserLibsData.xlsx文件后，有可能会出现xlsx文件不识别的问题。
可以尝试对UserLibsData.xlsx进行若干无效编辑后保存再尝试，无效编辑可以是对某个单元格增加一个空格再删除这个空格，保存文件即可。
但根本解决之法是使用Microsoft Excel软件进行编辑。


