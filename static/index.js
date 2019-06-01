let width = 640;
let height = 360;

// https://www.pyimagesearch.com/2015/11/30/detecting-machine-readable-zones-in-passport-images/
// https://www.pyimagesearch.com/2014/09/01/build-kick-ass-mobile-document-scanner-just-5-minutes/

let ocrRunning = false;

// whether streaming video from the camera.
let streaming = false;

let video = document.getElementById("video");
let stream = null;
let vc = null;

function startCamera() {
  if (streaming) return;
  navigator.mediaDevices.getUserMedia({
    video: {
      width: { min: 1280, ideal: 1920 },
      height: { min: 720, ideal: 1080 }    
    }, 
    audio: false})
    .then(function(s) {
      stream = s;
      video.srcObject = s;
      video.play();
    })
    .catch(function(err) {
    console.log("An error occured! " + err);
  });

  video.addEventListener("canplay", function(ev){
    if (!streaming) {
      height = video.videoHeight / (video.videoWidth/width);
      video.setAttribute("width", video.videoWidth);
      video.setAttribute("height", video.videoHeight);
      streaming = true;
      vc = new cv.VideoCapture(video);
    }
    startVideoProcessing();
  }, false);
}

let src = null;
let dstC1 = null;
let dstC2 = null;
let dstC3 = null;
let dstC4 = null;
let rectKernel = null;
let sqKernel = null;
let gray = null;
let blackhat = null;
let colorRed = null;
let colorGreen = null;
let consecutiveRoiFound = 0;
let time = Date.now()

function startVideoProcessing() {
  if (!streaming) { console.warn("Please startup your webcam"); return; }
  stopVideoProcessing();
  src = new cv.Mat(video.videoHeight, video.videoWidth, cv.CV_8UC4);
  dstC1 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC2 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC3 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC4 = new cv.Mat(height, width, cv.CV_8UC4);
  rectKernel = cv.getStructuringElement(cv.MORPH_RECT, {width: 13, height: 5});
  sqKernel = cv.getStructuringElement(cv.MORPH_RECT, {width: 21, height: 21});
  gray = new cv.Mat(height, width, cv.CV_8UC1);
  blackhat = new cv.Mat(height, width, cv.CV_8UC1);
  colorRed = new cv.Scalar(255, 0, 0);
  colorGreen = new cv.Scalar(0, 255, 0);
  requestAnimationFrame(processVideo);
}

function processVideo() {
  vc.read(src);
  cv.imshow("canvasOutput", myfilter(src));
  requestAnimationFrame(processVideo);
}

function stopVideoProcessing() {
  if (src != null && !src.isDeleted()) src.delete();
  if (dstC1 != null && !dstC1.isDeleted()) dstC1.delete();
  if (dstC2 != null && !dstC2.isDeleted()) dstC2.delete();
  if (dstC3 != null && !dstC3.isDeleted()) dstC3.delete();
  if (dstC4 != null && !dstC4.isDeleted()) dstC4.delete();
}

function stopCamera() {
  if (!streaming) return;
  stopVideoProcessing();
  document.getElementById("canvasOutput").getContext("2d").clearRect(0, 0, width, height);
  document.getElementById("canvasROI").getContext("2d").clearRect(0, 0, width, 100);
  video.pause();
  video.srcObject=null;
  stream.getVideoTracks()[0].stop();
  streaming = false;
}


function hughLines(src) {
  let lines = new cv.Mat();
  let color = colorRed;
  let left = -1;
  let leftVal = width;
  let right = -1;
  let rightVal = 0;
  let lower = -1;
  let lowerVal = height;
  let upper = -1;
  let upperVal = 0;
  cv.Canny(src, src, 50, 200, 3);
  cv.HoughLinesP(src, lines, 1, Math.PI / 180, 2, 70, 8);
  for (let i = 0; i < lines.rows; ++i) {
    let a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
    let b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);
    let diffX = Math.abs(a.x - b.x);
    let diffY = Math.abs(a.y - b.y);
    if ((a.x < 10) || (a.x > width-10) || (a.y < 10) || (a.y > height-10)) {
      continue;
    }
    if ((b.x < 10) || (b.x > width-10) || (b.y < 10) || (b.y > height-10)) {
      continue;
    }
    // vertical line
    if (diffX < 20) {
      if (a.x < width * 0.3) {
        if (a.x < leftVal) {
          left = i;
          leftVal = a.x;
        }
        cv.line(dstC1, a, b, colorRed, 1);          
      }
      if (a.x > width * 0.7) {
        if (a.x > rightVal) {
          right = i;
          rightVal = a.x;
        }
        cv.line(dstC1, a, b, colorRed, 1);          
      }
    }
    // horizontal line
    if (diffY < 10) {
      if (a.y < height * 0.3) {
        if (a.y < lowerVal) {
          lower = i;
          lowerVal = a.y;
        }
        cv.line(dstC1, a, b, colorRed, 1);          
      }
      if (a.y > height * 0.7) {
        if (a.y > upperVal) {
          upper = i;
          upperVal = a.y;
        }
        cv.line(dstC1, a, b, colorRed, 1);          
      }
    }
    //a.delete();
    //b.delete();
  }
  if (left >= 0 && right >= 0 && lower >= 0 && upper >= 0) {
    let i = left;
    let left_a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
    let left_b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);
    i = right;
    right_a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
    right_b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);
    i = lower;
    lower_a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
    lower_b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);
    i = upper;
    upper_a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
    upper_b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);

    // check 2/3 ration
    let length = right_a.x - left_a.x
    let height = upper_a.y - lower_a.y
    let ratio = length / height
    if (ratio > 1.3 && ratio < 1.7) {
      color = colorGreen;
    }
    cv.line(dstC1, left_a, left_b, color, 3);
    cv.line(dstC1, right_a, right_b, color, 3);
    cv.line(dstC1, lower_a, lower_b, color, 3);
    cv.line(dstC1, upper_a, upper_b, color, 3);
  }

  lines.delete();
}

function myfilter(orig) {
  let dsize = new cv.Size(width, height);
  cv.resize(orig, dstC1, dsize, 0, 0, cv.INTER_AREA);

  cv.cvtColor(dstC1, dstC1, cv.COLOR_RGBA2RGB);
  cv.cvtColor(dstC1, dstC3, cv.COLOR_RGB2GRAY);

  hughLines(dstC3);
  //return dstC1;

  cv.Canny(dstC3, dstC4, 200, 150);
  cv.morphologyEx(dstC4, dstC3, cv.MORPH_BLACKHAT, rectKernel);
  cv.morphologyEx(dstC3, dstC3, cv.MORPH_CLOSE, rectKernel);
  cv.threshold(dstC3, dstC3, 0, 255, cv.THRESH_BINARY | cv.THRESH_OTSU);

  const contoursCard = new cv.MatVector();
  const contoursRoi = new cv.MatVector();
  const hierarchy = new cv.Mat();
  // RETR_EXTERNAL, RETR_LIST
  cv.findContours(dstC3, contoursRoi, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
    
  let foundRoi = 0;
  let detectedRoi = 0;
  let minX = width;
  let maxX = 0;
  let minY = height;
  let maxY = 0;

  for (let i = 0; i < contoursRoi.size(); ++i) {
    const cnt = contoursRoi.get(i);
    let rect = cv.boundingRect(cnt);
    if (rect.width < 300 || rect.height < 8 || rect.height > 50) {
      continue;
    }
    detectedRoi += 1;
    let area = cv.contourArea(cnt, false);
    let rectArea = rect.width * rect.height;
    let extent = area / rectArea;
    if (extent < 0.5) {
      continue;
    }
    foundRoi += 1;
    minX = Math.min(minX, rect.x);
    maxX = Math.max(maxX, rect.x + rect.width);
    minY = Math.min(minY, rect.y);
    maxY = Math.max(maxY, rect.y + rect.height);
  }
  if (detectedRoi > 1) {
    if (foundRoi > 1) {
      consecutiveRoiFound += 1;
    } else {
      consecutiveRoiFound = 0;      
    }
    let minP = new cv.Point(minX - 10, minY - 10);
    let maxP = new cv.Point(maxX + 20, maxY + 10);
    console.log("******** " + minX + ", " + minY + ", " + maxX + ", " + maxY + ", ");
    cv.rectangle(dstC1, minP, maxP, (consecutiveRoiFound > 12) ? colorGreen : colorRed, 2);
    
    if (consecutiveRoiFound > 12) {
      if(!ocrRunning){
        let x = (minX - 10) * video.videoWidth / width;
        let w = (maxX - minX  + 30) * video.videoWidth / width;
        let y = (minY - 10) * video.videoHeight / height;
        let h = (maxY - minY  + 20) * video.videoHeight / height;
        let rect = new cv.Rect(x, y, w, h);
        ocrRunning = true;
        let roi = new cv.Mat();
        roi = src.roi(rect);
        cv.imshow("canvasRoi", roi);
        let img = document.getElementById("canvasRoi").getContext('2d');
        //window.Tesseract = Tesseract.create({lang: 'en'});
        window.Tesseract = Tesseract.create({
          workerPath: 'http://localhost:8000/tesseract/worker.js',
          langPath: 'http://localhost:8000/tesseract/lang/',
          corePath: 'http://localhost:8000/tesseract/tesseract_core.js',
        });
  
        Tesseract.recognize(img, {
            lang: 'spa'
          })
          .progress(message => console.log(message))
          .catch(err => {console.error(err); ocrRunning = false;})
          .then(result => {console.log(result); ocrRunning = false;})
          .finally(resultOrError => {console.log(resultOrError); ocrRunning = false;})
  
        roi.delete();
      }
    }
  } else {
    consecutiveRoiFound = 0;
  }
  //console.log("Time" + (Date.now() - time));
  time = Date.now()
  return dstC1;
}

function opencvIsReady() {
  console.log('OpenCV.js is ready');
  startCamera();
}

