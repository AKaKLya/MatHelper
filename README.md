# MatHelper

这个插件可以在虚幻商城里面下载，名字就叫做 "MatHelper".

5.3版本的插件已经完结， V6.1插件版本 在细节面板中添加了一个按钮，这是通过复制引擎原有的细节面板的代码 并且做了改动来实现的.

但是5.4引擎原有的细节面板的代码有所改动，并且细节面板所依赖的 "RenderCore"模块 也有改动，

所以 5.3 和 5.4 的插件代码不能通用，直接放弃5.3的插件更新.  也没什么好更新的了，我原本想要添加的功能 都添加上了.


## 2024.3.16

Add [Create Material Instance] Button

Add [Open Refraction] Button

Change [Material] Object Color to Red in ContentBrowser.

Add [Fix Function Node] Button .

Add [Edit Group Text] Button

## 2024.3.20

FixBug

## 2024.3.22

Use DataAsset to define something.

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/2ffd15de-3c32-415c-85be-1bab28354c23)


## 2024.3.24  
Update to V4.0

Extend Palette Tool

![MH4 0](https://github.com/AKaKLya/MatHelper/assets/67385510/f4098347-383d-4a80-919a-dd5b1996ca0b)

## 2024.3.29 V4.4
不需要再按 ? 键

增加Niagara播放按钮

在世界大纲里勾选Niagara旁边的勾之后，会在这个NiagaraActor里添加一个Tag.

按下播放按钮时，检测是否存在这个Tag, 如果存在 就播放.

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/0288b9f7-5df1-44cd-bd6c-54c824268f6c)

## V5.0
Tool 工具栏: 

增加 资产解锁 Lock/UnLock 功能， 

选中资产 -> 点击Lock ，这个资产就不能被右键Export导出.

反之，UnLock 可以解锁资产，不能导出的资产 也能导出了. 例如 虚幻争霸的贴图, UnLock后就可以导出.

增加 中英语言切换、 重启引擎


![Tool](https://github.com/AKaKLya/MatHelper/assets/67385510/b671552f-21a5-4680-9a17-a0df048bcdc9)


## V5.1

创建节点时,不再污染Windows剪贴板.

旧版:当前剪贴板的复制内容为 "FFFF",创建节点后，会把节点的代码放到剪贴板里，"FFFF"被节点代码取代. 按下Ctrl+V时 节点代码将会被粘贴.

新版:创建节点时，备份剪贴板的内容--->将节点代码放进剪贴板--->创建节点--->恢复备份的剪贴板内容，按下Ctrl+V时，"FFFF"将会被粘贴.

创建节点时,自动跳转到最新创建的节点.


## V5.1.1

修复 [Create Material Instance 2] 造成的资产不显示的问题.


## V6.0 

可自定义配置的[Mask Pin]

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/d964a077-522e-4564-b3a1-0a7ac710144a)

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/f602eb33-7b6b-4634-be03-de1baed0d14b)

## V6.1 

增加 [材质实例编辑器] 开启/关闭所有参数 按钮,

点击 开启所有参数，再点 关闭所有参数. 有时会卡住 多点两下就行.


![image](https://github.com/AKaKLya/MatHelper/assets/67385510/5057d5ad-8d1e-4525-8788-c881fb9d4119)


## V6.2 场景预览窗口

可以在 材质插件窗口里打开，也可以在菜单栏里打开.

Tool菜单栏的启动方法可以在任何一个编辑器中 创建预览窗口，例如:Niagara编辑器,蓝图编辑器...


![image](https://github.com/AKaKLya/MatHelper/assets/67385510/0ed47448-e31e-4382-bc3a-ffcbee9cfadc)

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/350fedc3-ddb6-421a-ad40-39cf31cb8a1d)



![image](https://github.com/AKaKLya/MatHelper/assets/67385510/273de50a-e298-44a8-8d92-228085c4cfe6)

![image](https://github.com/AKaKLya/MatHelper/assets/67385510/10bea428-96f8-4b54-a8fa-57d27c96def7)

