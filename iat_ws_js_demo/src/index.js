/**
 * Created by lycheng on 2019/8/1.
 *
 * 语音听写流式 WebAPI 接口调用示例 接口文档（必看）：https://doc.xfyun.cn/rest_api/语音听写（流式版）.html
 * webapi 听写服务参考帖子（必看）：http://bbs.xfyun.cn/forum.php?mod=viewthread&tid=38947&extra=
 * 语音听写流式WebAPI 服务，热词使用方式：登陆开放平台https://www.xfyun.cn/后，找到控制台--我的应用---语音听写---个性化热词，上传热词
 * 注意：热词只能在识别的时候会增加热词的识别权重，需要注意的是增加相应词条的识别率，但并不是绝对的，具体效果以您测试为准。
 * 错误码链接：
 * https://www.xfyun.cn/doc/asr/voicedictation/API.html#%E9%94%99%E8%AF%AF%E7%A0%81
 * https://www.xfyun.cn/document/error-code （code返回错误码时必看）
 * 语音听写流式WebAPI 服务，方言或小语种试用方法：登陆开放平台https://www.xfyun.cn/后，在控制台--语音听写（流式）--方言/语种处添加
 * 添加后会显示该方言/语种的参数值
 *
 */

/**
 * Worker(): https://developer.mozilla.org/zh-CN/docs/Web/API/Worker/Worker
 * Worker() 构造函数创建一个 Worker 对象，该对象执行指定的URL脚本。
 *
 * 使用 Web Workers: https://developer.mozilla.org/zh-CN/docs/Web/API/Web_Workers_API/Using_web_workers
 * Web Worker为Web内容在后台线程中运行脚本提供了一种简单的方法。线程可以执行任务而不干扰用户界面。
 * 一旦创建， 一个worker 可以将消息发送到创建它的JavaScript代码, 通过将消息发布到该代码指定的事件处理程序
 *
 * @type {Worker}
 */
// 音频转码worker
let recorderWorker = new Worker('./transformpcm.worker.js')

// 记录处理的缓存音频
let buffer = []

/**
 * AudioContext: https://developer.mozilla.org/zh-CN/docs/Web/API/AudioContext
 * AudioContext接口表示由音频模块连接而成的音频处理图，每个模块对应一个AudioNode。
 * AudioContext可以控制它所包含的节点的创建，以及音频处理、解码操作的执行。
 * 做任何事情之前都要先创建AudioContext对象，因为一切都发生在这个环境之中。
 *
 * @type {{prototype: AudioContext; new(contextOptions?: AudioContextOptions): AudioContext} | *}
 */
let AudioContext = window.AudioContext || window.webkitAudioContext

let notSupportTip = '请试用chrome浏览器且域名为localhost或127.0.0.1测试'

/**
 * Navigator: https://developer.mozilla.org/zh-CN/docs/Web/API/Navigator
 * Navigator 接口表示用户代理的状态和标识。 它允许脚本查询它和注册自己进行一些活动。
 *
 * navigator.getUserMedia: https://developer.mozilla.org/zh-CN/docs/Web/API/Navigator/getUserMedia
 * Navigator.getUserMedia()方法提醒用户需要使用音频（0或者1）和（0或者1）视频输入设备，
 * 比如相机，屏幕共享，或者麦克风。如果用户给予许可，
 *
 * @type {((constraints: MediaStreamConstraints, successCallback: NavigatorUserMediaSuccessCallback, errorCallback: NavigatorUserMediaErrorCallback) => void) | *}
 */
navigator.getUserMedia = navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia || navigator.msGetUserMedia

/**
 * Worker.onmessage: https://developer.mozilla.org/zh-CN/docs/Web/API/Worker/onmessage
 * Worker 接口的onmessage属性表示一个EventHandler事件处理函数，
 * 当message 事件发生时，该函数被调用。这些事件所属MessageEvent类型，
 * 且当Worker子线程返回一条消息时被调用
 * （比如：从DedicatedWorkerGlobalScope.postMessage函数发出的信息）
 *
 * @param e
 */
recorderWorker.onmessage = function (e) {
  /**
   *  省略号...是展开语法：https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Operators/Spread_syntax
   *  展开语法(Spread syntax), 可以在函数调用/数组构造时, 将数组表达式或者string在语法层面展开；
   *  还可以在构造字面量对象时, 将对象表达式按key-value的方式展开。
   *
   */
  buffer.push(...e.data.buffer)
}


class IatRecorder {
  /**
   * constructor: https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Classes/constructor
   *
   * @param config
   */
  constructor (config) {
    this.config = config
    this.state = 'ing'
    this.language = config.language || 'zh_cn'//默认中文
    this.accent = config.accent || 'mandarin'//默认普通话

    //以下信息在控制台-我的应用-语音听写（流式版）页面获取
    this.appId = '5da1e412'
    this.apiKey = 'f1cdd05afbd977abc46360691be9ac40'
    this.apiSecret = 'd18345701ebbe9280bd7dba58b108f9a'
  }

  /**
   * 录音启动函数
   */
  start () {
    //启动录音服务前先stop终止，以防启动前有录音服务正在进行
    this.stop()

    //用户允许使用音频输入设备，且AudioContext环境已创建，进行下一步
    if (navigator.getUserMedia && AudioContext) {
      //将录音状态码置为ing，录音正在进行
      this.state = 'ing'

      //this.recorder不存在，进行下一步
      if (!this.recorder) {

        //声明音频模块环境
        var context = new AudioContext()
        this.context = context

        /**
         * AudioContext.createScriptProcessor(): https://developer.mozilla.org/zh-CN/docs/Web/API/AudioContext/createScriptProcessor
         * AudioContext 接口的createScriptProcessor() 方法创建一个ScriptProcessorNode 用于通过JavaScript直接处理音频.
         * 简而言之，this.recorder被用于处理音频
         *
         * ScriptProcessorNode 接口允许使用JavaScript生成、处理、分析音频.
         * @type {ScriptProcessorNode}
         */
        this.recorder = context.createScriptProcessor(0, 1, 1)

        /**
         * 箭头函数 =>
         * link:
         *  https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Functions/Arrow_functions
         *  https://blog.fundebug.com/2017/05/25/arrow-function-for-beginner/
         *
         * 此处定义了getMediaSuccess函数，参数为stream
         * @param stream
         */
        var getMediaSuccess = (stream) => {
          /**
           * AudioContext.createMediaStreamSource(): https://developer.mozilla.org/zh-CN/docs/Web/API/AudioContext/createMediaStreamSource
           *  AudioContext接口的createMediaStreamSource()方法用于创建一个新的MediaStreamAudioSourceNode 对象,
           *  需要传入一个媒体流对象(MediaStream对象)(可以从 navigator.getUserMedia 获得MediaStream对象实例),
           *  然后来自MediaStream的音频就可以被播放和操作。
           *
           *  ∴ 定义的mediaStream对象可以作为媒体流对象被操作
           *
           * @type {MediaStreamAudioSourceNode}
           */
          var mediaStream = this.context.createMediaStreamSource(stream)
          this.mediaStream = mediaStream

          /**
           * 对于每一个声道和样本帧，在把结果当成输出样本之前，
           * onaudioprocess方法关联audioProcessingEvent，用它来遍历每输入流的每一个声道。
           *
           * @param e
           */
          this.recorder.onaudioprocess = (e) => {
            //随着音频的输入，发送音频缓存数据
            this.sendData(e.inputBuffer.getChannelData(0))
          }

          //连接websocket
          this.connectWebsocket()
        }

        /**
         * 此处定义了getMediaFail函数，参数为e
         *
         * @param e
         */
        var getMediaFail = (e) => {
          this.recorder = null
          this.mediaStream = null
          this.context = null
          console.log('请求麦克风失败')
        }

        /**
         * Navigator.mediaDevices: https://developer.mozilla.org/zh-CN/docs/Web/API/Navigator/mediaDevices
         * MediaDevices.getUserMedia(): https://developer.mozilla.org/zh-CN/docs/Web/API/MediaDevices/getUserMedia
         *
         * mediaDevices 是 Navigator 只读属性，返回一个 MediaDevices 对象，
         * 该对象可提供对相机和麦克风等媒体输入设备的连接访问，也包括屏幕共享。
         *
         * MediaDevices.getUserMedia() 会提示用户给予使用媒体输入的许可
         *
         */
        //如果用户成功给予了使用媒体输入的许可
        if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
          navigator.mediaDevices.getUserMedia({
            audio: true,
            video: false
          }).then((stream) => {
            //调用getMediaSuccess函数，建立websocket连接
            getMediaSuccess(stream)
          }).catch((e) => {
            //发生异常，调用getMediaFail重新进行连接
            getMediaFail(e)
          })
          //如果用户没有给予使用媒体输入的许可
        } else {
          //再次请求用户的许可
          navigator.getUserMedia({
            audio: true,
            video: false
          }, (stream) => {
            //获得用户的许可之后，再调用getMediaSuccess函数，尝试建立websocket连接
            getMediaSuccess(stream)
          }, function (e) {
            //建立连接失败，发生异常，调用getMediaFail重新进行websocket连接
            getMediaFail(e)
          })
        }
      } else {
        //this.recorder已存在，直接连接WebSocket服务
        this.connectWebsocket()
      }
    } else {
      //录音环境不支持，报错提醒用户
      var isChrome = navigator.userAgent.toLowerCase().match(/chrome/)
      alert(notSupportTip)
    }
  }

  /**
   * 录音终止函数
   */
  stop () {
    //状态码置为end
    this.state = 'end'
    //媒体流对象断开连接，录音服务断开连接
    try {
      this.mediaStream.disconnect(this.recorder)
      this.recorder.disconnect()
    } catch (e) {}
  }

  /**
   * 发送音频缓存数据给worker
   *
   * @param buffer
   */
  sendData (buffer) {
    /**
     * workers和主线程间的数据传递通过这样的消息机制进行——双方都使用postMessage()方法发送各自的消息，
     * 使用onmessage事件处理函数来响应消息（消息被包含在Message事件的data属性中）。
     * 这个过程中数据并不是被共享而是被复制。
     *
     * 这里被发送的messgae是一个json对象
     */
    recorderWorker.postMessage({
      command: 'transform',
      buffer: buffer
    })
  }

  /**
   * WebSockets: https://developer.mozilla.org/zh-CN/docs/Glossary/WebSockets
   *  WebSocket 是一种在客户端与服务器之间保持TCP长连接的网络协议，这样它们就可以随时进行信息交换。
   *  虽然任何客户端或服务器上的应用都可以使用WebSocket，但原则上还是指浏览器与服务器之间使用。
   *  通过WebSocket，服务器可以直接向客户端发送数据，而无须客户端周期性的请求服务器，以动态更新数据内容。
   *
   *
   * WebSocket(): https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/WebSocket
   *  WebSocket()构造函器会返回一个 WebSocket 对象。
   *
   * @returns {null}
   */
  connectWebsocket () {
    //一大堆连接讯飞语音听写服务器要用到的变量
    var url = 'wss://iat-api.xfyun.cn/v2/iat'
    var host = 'iat-api.xfyun.cn'
    var apiKey = this.apiKey
    var apiSecret = this.apiSecret
    var date = new Date().toGMTString()
    var algorithm = 'hmac-sha256'
    var headers = 'host date request-line'
    var signatureOrigin = `host: ${host}\ndate: ${date}\nGET /v2/iat HTTP/1.1`
    var signatureSha = CryptoJS.HmacSHA256(signatureOrigin, apiSecret)
    var signature = CryptoJS.enc.Base64.stringify(signatureSha)
    var authorizationOrigin = `api_key="${apiKey}", algorithm="${algorithm}", headers="${headers}", signature="${signature}"`
    var authorization = btoa(authorizationOrigin)

    //这是WebSocket服务器将响应的URL
    url = `${url}?authorization=${authorization}&date=${date}&host=${host}`

    /**
     * window: https://developer.mozilla.org/zh-CN/docs/Web/API/Window
     *  这里是在判断当前浏览器是否可以建立WebSocket连接
     */
    if ('WebSocket' in window) {
      //若可以建立WebSocket连接，实例化一个WebSocket对象: this.ws
      this.ws = new WebSocket(url)
    } else if ('MozWebSocket' in window) {
      // 火狐浏览器最新的WebSocket对象是MozWebSocket
      this.ws = new MozWebSocket(url)
    } else {
      alert(notSupportTip)
      return null
    }

    /**
     * WebSocket.onopen: https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/onopen
     *  WebSocket.onopen属性定义一个事件处理程序，当WebSocket 的连接状态readyState 变为“OPEN”时调用;
     *  这意味着当前连接已经准备好发送和接受数据。
     *  这个事件处理程序通过 事件（建立连接时）触发。
     *
     *  onopen return An EventListener. -- e
     *
     * @param e
     */
    this.ws.onopen = (e) => {
      /**
       * 没找到中文解释
       * AudioNode.connect(AudioNode)
       *  Allows to connect one output of this node to one input of another node.
       * 大概意思是将音频处理模块与媒体流对象连接起来？
       */
      this.mediaStream.connect(this.recorder)

      /**
       * AudioContext.destination: https://developer.mozilla.org/zh-CN/docs/Web/API/AudioContext/destination
       * AudioContext的destination属性返回一个AudioDestinationNode
       * 表示context中所有音频（节点）的最终目标节点，一般是音频渲染设备，比如扬声器。
       */
      this.recorder.connect(this.context.destination)

      /**
       * window.setTimeout: https://developer.mozilla.org/zh-CN/docs/Web/API/Window/setTimeout
       * setTimeout()方法设置一个定时器，该定时器在定时器到期后执行一个函数或指定的一段代码。
       */
      setTimeout(() => {
        //以上准备操作完成后，执行wsOpened函数，通过websocket连接向服务器发送音频数据
        this.wsOpened(e)
      }, 500)

      //this.config.onStart没有调用onStart函数，this.config.onStart(e)才调用了onStart函数
      //实现录音时间在页面上的实时变化
      this.config.onStart && this.config.onStart(e)
    }

    /**
     * WebSocket.onmessage: https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/onmessage
     * WebSocket.onmessage 属性是一个当收到来自服务器的消息时被调用的 EventHandler。
     * 它由一个MessageEvent调用。
     *
     * @param e
     */
    this.ws.onmessage = (e) => {
      //收到服务器返回的消息时，调用this.config.onMessage方法，将结果处理后显示到页面上
      this.config.onMessage && this.config.onMessage(e)
      //收到服务器传回的结果数据后，解析json数据串，判断是否识别结束
      this.wsOnMessage(e)
    }


    /**
     * WebSocket.onerror: https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/onerror
     * 在 WebSocket.onerror 属性中，你可以定义一个发生错误时执行的回调函数，此事件的事件名为"error"
     *
     * websocket连接发生错误时，停止录音，调用this.config.onError方法
     * @param e
     */
    this.ws.onerror = (e) => {
      this.stop()
      this.config.onError && this.config.onError(e)
    }

    /**
     * WebSocket.onclose: https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/onclose
     * WebSocket.onclose 属性返回一个事件监听器，
     * 这个事件监听器将在 WebSocket 连接的readyState 变为 CLOSED时被调用，
     * 它接收一个名字为“close”的 CloseEvent 事件。
     *
     * WebSocket连接关闭时调用此函数，停止录音并调用this.config.onClose函数
     * @param e
     */
    this.ws.onclose = (e) => {
      this.stop()
      this.config.onClose && this.config.onClose(e)
    }
  }

  /**
   * WebSocket已链接并且可以通讯
   * 通过websocket发送音频数据至讯飞服务器
   */
  wsOpened () {
    /**
     * WebSocket.readyState: https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/readyState
     * 返回当前 WebSocket 的链接状态，只读。
     * 0 - 正在链接中
     * 1 - 已链接并且可以通讯
     * 2 - 连接正在关闭
     * 3 - 连接已关闭或者没有链接成功
     *
     */
    if (this.ws.readyState !== 1) {
      return
    }

    /**
     * splice() 方法通过删除或替换现有元素或原地添加新的元素来修改数组,
     * 并以数组形式返回被修改的内容。此方法会改变原数组。
     * @type {*[]}
     */
        //audioDate取得音频缓存buffer数组中的前1280个
    var audioData = buffer.splice(0, 1280)
    console.log('audioData length:', audioData.length)

    //一大堆要通过WebSocket链接向服务器发送的参数
    var params = {
      'common': {
        'app_id': this.appId
      },
      'business': {
        'language': this.language, //小语种可在控制台--语音听写（流式）--方言/语种处添加试用
        'domain': 'iat',
        'accent': this.accent, //中文方言可在控制台--语音听写（流式）--方言/语种处添加试用
        'vad_eos': 5000,
        'dwa': 'wpgs' //为使该功能生效，需到控制台开通动态修正功能（该功能免费）
      },
      'data': {
        'status': 0, //音频的状态 0:第一帧音频 1:中间的音频 2:最后一帧音频，最后一帧必须要发送
        'format': 'audio/L16;rate=16000',
        'encoding': 'raw',
        //音频数据被转换为base64编码格式
        'audio': this.ArrayBufferToBase64(audioData)
      }
    }

    /**
     * WebSocket.send(): https://developer.mozilla.org/zh-CN/docs/Web/API/WebSocket/send
     * WebSocket.send() 方法将需要通过 WebSocket 链接传输至服务器的数据排入队列，
     * 并根据所需要传输的data bytes的大小来增加 bufferedAmount的值 。
     * 若数据无法传输（例如数据需要缓存而缓冲区已满）时，套接字会自行关闭。
     *
     */
    this.ws.send(JSON.stringify(params))

    /**
     * window.setInterval: https://developer.mozilla.org/zh-CN/docs/Web/API/Window/setInterval
     * setInterval() 方法重复调用一个函数或执行一个代码段，在每次调用之间具有固定的时间延迟。
     *
     * @type {number}
     */
    //这里每隔40ms从buffer中取出音频通过websocket发送至服务器
    this.handlerInterval = setInterval(() => {
      // websocket未连接
      if (this.ws.readyState !== 1) {
        // clearInterval() 方法可取消先前通过 setInterval() 设置的重复定时任务。
        clearInterval(this.handlerInterval)
        return
      }

      //如果音频缓存为空
      if (buffer.length === 0) {
        //录音状态置为end，终止录音
        if (this.state === 'end') {
          //通过websocket发送数据到服务器
          this.ws.send(JSON.stringify({
            'data': {
              'status': 2, //音频的状态 0:第一帧音频 1:中间的音频 2:最后一帧音频，最后一帧必须要发送
              'format': 'audio/L16;rate=16000',
              'encoding': 'raw',
              'audio': '' //啥也没录到
            }
          }))
          // clearInterval() 方法可取消先前通过 setInterval() 设置的重复定时任务。
          clearInterval(this.handlerInterval)
        }
        return false
      }

      //音频缓存不为空，继续取得前1280个数据
      audioData = buffer.splice(0, 1280)

      // 中间帧
      this.ws.send(JSON.stringify({
        'data': {
          'status': 1,//音频的状态 0:第一帧音频 1:中间的音频 2:最后一帧音频，最后一帧必须要发送
          'format': 'audio/L16;rate=16000',
          'encoding': 'raw',
          //音频数据被转换为base64编码格式
          'audio': this.ArrayBufferToBase64(audioData)
        }
      }))
    }, 40)
  }

  /**
   * 收到服务器传回的结果数据后，解析json数据串，判断是否识别结束
   * @param e
   */
  wsOnMessage (e) {
    let jsonData = JSON.parse(e.data)
    // 识别结束
    // code是返回码：0表示成功，其它表示异常
    // status是识别结果是否结束的标识：1是识别中，2是识别结束
    if (jsonData.code === 0 && jsonData.data.status === 2) {
      //关闭websocket连接
      this.ws.close()
    }

    //如果识别异常，关闭websocket连接，并且输出异常
    if (jsonData.code !== 0) {
      this.ws.close()
      console.log(`${jsonData.code}:${jsonData.message}`)
    }
  }

  /**
   * 将音频缓存数组转换为Base64编码格式的数据
   * @param buffer
   * @returns {string}
   * @constructor
   */
  ArrayBufferToBase64 (buffer) {
    var binary = ''
    var bytes = new Uint8Array(buffer)
    var len = bytes.byteLength
    for (var i = 0; i < len; i++) {
      binary += String.fromCharCode(bytes[i])
    }
    return window.btoa(binary)
  }
}

class IatTaste {
  /**
   * constructor :https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Classes/constructor
   * constructor 是一种用于创建和初始化class创建的对象的特殊方法。
   * 在一个类中只能有一个名为 “constructor” 的特殊方法，
   * 在一个构造方法中可以使用super关键字来调用一个父类的构造方法。
   *
   */
  constructor () {
    //创建一个IatRecorder类型的实例
    var iatRecorder = new IatRecorder({
      /**
       * 箭头函数 =>
       * 没有参数的函数应该写成一对圆括号。  () => {函数声明}
       * link:
       *  https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Functions/Arrow_functions
       *  https://blog.fundebug.com/2017/05/25/arrow-function-for-beginner/
       *
       */

      /**
       *  websocket关闭连接，终止录音
       */
      onClose: () => {
        this.stop()
        this.reset()
      },

      /**
       *  websocket连接发生错误，终止录音，并报错
       */
      onError: (data) => {
        this.stop()
        this.reset()
        alert('WebSocket连接失败')
      },

      //服务器返回数据时，将结果转换为json，然后调用this.setResult方法将结果显示出来
      onMessage: (e) => {
        let str = ''
        let jsonData = JSON.parse(e.data)
        if (jsonData.data && jsonData.data.result) {
          this.setResult(jsonData.data.result)
        }
      },

      /**
       * 实现录音时间在页面上的实时变化
       */
      onStart: () => {
        $('hr').addClass('hr')
        var dialect = $('.dialect-select').find('option:selected').text()
        //前端UI变化
        $('.taste-content').css('display', 'none')
        $('.start-taste').addClass('flex-display-1')
        $('.dialect-select').css('display', 'none')
        $('.start-button').text('结束识别')
        $('.time-box').addClass('flex-display-1')
        $('.dialect').text(dialect).css('display', 'inline-block')

        /**
         * 启动录音计时函数
         * html: <span class="used-time">00: 00</span>
         * used-time是当前录音时长，把这个时长作为参数传给counterDown
         * 不过这个'used-time'参数好像并没有用到
         * 下面定义了一个DOM元素对象： this.counterDownDOM = $('.used-time')
         * counterDown中直接对this.counterDownDOM的text进行修改以达到前端时间显示的变化
         */
        this.counterDown($('.used-time'))
      }
    })

    this.iatRecorder = iatRecorder

    //html: <span class="used-time">00: 00</span> 指向该DOM对象
    this.counterDownDOM = $('.used-time')

    //录音时间初始化为0
    this.counterDownTime = 0

    this.text = {
      start: '开始识别',
      stop: '结束识别'
    }

    //语音听写识别结果字符串
    this.resultText = ''
  }

  start () {
    this.iatRecorder.start()
  }

  stop () {
    $('hr').removeClass('hr')
    this.iatRecorder.stop()
  }

  /**
   * 重置
   */
  reset () {
    //将录音计时时间初始化为0
    this.counterDownTime = 0

    //调用内置的clearTimeout()方法取消了先前通过调用setTimeout()建立的定时器。
    clearTimeout(this.counterDownTimeout)

    //录音音频缓存数组置为空
    buffer = []

    //前端UI调整为初始状态
    $('.time-box').removeClass('flex-display-1').css('display', 'none')
    $('.start-button').text(this.text.start)
    $('.dialect').css('display', 'none')
    $('.dialect-select').css('display', 'inline-block')
  }

  /**
   * 初始化
   */
  init () {
    /**
     * this: https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Operators/this
     *
     * @type {IatTaste}
     */
    let self = this

    //给按钮设置点击事件
    $('#taste_button').click(function () {
      //用户允许使用音频输入设备，AudioContext环境已创建，且音频转码worker已创建
      if (navigator.getUserMedia && AudioContext && recorderWorker) {
        //一切准备就绪，启动录音
        self.start()
      } else {
        //没准备好，报错
        alert(notSupportTip)
      }
    })

    $('.start-button').click(function () {
      if ($(this).text() === self.text.start) {
        $('#result_output').text('')
        self.resultText = ''
        self.start()
      } else {
        self.reset()
        self.stop()
      }
    })
  }

  /**
   * 将服务器返回的结果数据data进行解析拼接后显示到页面上
   * @param data
   */
  setResult (data) {
    var str = ''
    var resultStr = ''
    let ws = data.ws
    for (let i = 0; i < ws.length; i++) {
      str = str + ws[i].cw[0].w
    }
    // 开启wpgs会有此字段(前提：在控制台开通动态修正功能)
    // 取值为 "apd"时表示该片结果是追加到前面的最终结果；取值为"rpl" 时表示替换前面的部分结果，替换范围为rg字段
    if (data.pgs === 'apd') {
      this.resultText = $('#result_output').text()
    }
    resultStr = this.resultText + str
    $('#result_output').text(resultStr)
  }

  /**
   * 录音计时函数
   * @returns {boolean}
   */
  counterDown () {
    //录音时间达到60s时停止录音
    if (this.counterDownTime === 60) {
      this.counterDownDOM.text('01: 00')
      this.stop()
    } else if (this.counterDownTime > 60) {
      //大于60s就重置
      this.reset()
      return false
    } else if (this.counterDownTime >= 0 && this.counterDownTime < 10) {
      this.counterDownDOM.text('00: 0' + this.counterDownTime)
    } else {
      this.counterDownDOM.text('00: ' + this.counterDownTime)
    }
    this.counterDownTime++

    /**
     * window.setTimeout: https://developer.mozilla.org/zh-CN/docs/Web/API/Window/setTimeout
     * setTimeout()方法设置一个定时器，该定时器在定时器到期后执行一个函数或指定的一段代码。
     *
     * 这里每隔1秒调用一次counterDown录音计时函数
     */
    this.counterDownTimeout = setTimeout(() => {
      this.counterDown()
    }, 1000)
  }
}
var iatTaste = new IatTaste()
iatTaste.init()
