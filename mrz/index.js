let width = 640;
let height = 360;
let contour_ratio = 2;

// https://www.pyimagesearch.com/2015/11/30/detecting-machine-readable-zones-in-passport-images/
// https://www.pyimagesearch.com/2014/09/01/build-kick-ass-mobile-document-scanner-just-5-minutes/

// whether streaming video from the camera.
let streaming = false;

let video = document.getElementById("video");
let stream = null;
let vc = null;
let contour_corners = null;
let contour_time = 0;
let contour_width = 0;
let contour_height = 0;


function opencvIsReady() {
  console.log('OpenCV.js is ready');
  startCamera();
}


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
let dstC5 = null;
let rectKernel = null;
let sqKernel = null;
let colorRed = null;
let colorGreen = null;


function startVideoProcessing() {
  if (!streaming) { console.warn("Please startup your webcam"); return; }
  stopVideoProcessing();
  src = new cv.Mat(video.videoHeight, video.videoWidth, cv.CV_8UC4);
  dstC1 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC2 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC3 = new cv.Mat(height, width, cv.CV_8UC3);
  dstC4 = new cv.Mat(height, width, cv.CV_8UC4);
  dstC5 = new cv.Mat(height, width, cv.CV_8UC3);
  rectKernel = cv.getStructuringElement(cv.MORPH_RECT, {width: 13, height: 5});
  sqKernel = cv.getStructuringElement(cv.MORPH_RECT, {width: 21, height: 21});
  colorRed = new cv.Scalar(255, 0, 0, 255);
  colorGreen = new cv.Scalar(0, 255, 0, 255);
  requestAnimationFrame(processVideo);
}

function processVideo() {
  vc.read(src);
  cv.imshow("canvas_webcam", detection(src));
  requestAnimationFrame(processVideo);
}

function stopVideoProcessing() {
  if (src != null && !src.isDeleted()) src.delete();
  if (dstC1 != null && !dstC1.isDeleted()) dstC1.delete();
  if (dstC2 != null && !dstC2.isDeleted()) dstC2.delete();
  if (dstC3 != null && !dstC3.isDeleted()) dstC3.delete();
  if (dstC4 != null && !dstC4.isDeleted()) dstC4.delete();
  if (dstC5 != null && !dstC5.isDeleted()) dstC5.delete();
}

function stopCamera() {
  if (!streaming) return;
  stopVideoProcessing();
  document.getElementById("canvas_webcam").getContext("2d").clearRect(0, 0, width, height);
  document.getElementById("canvas_roi").getContext("2d").clearRect(0, 0, width, 100);
  video.pause();
  video.srcObject=null;
  stream.getVideoTracks()[0].stop();
  streaming = false;
}

let found = false;
function detection(orig) {
  let dsize = new cv.Size(width, height);
  cv.resize(orig, dstC1, dsize, 0, 0, cv.INTER_LINEAR); // cv.INTER_AREA
  cv.cvtColor(dstC1, dstC3, cv.COLOR_RGBA2GRAY);
  cv.equalizeHist(dstC3, dstC3);

  if (!found) {
    var corners = detectcontour(dstC3);
    if (contour_corners != null && corners == null) {
      if (contour_time == 0) {
        contour_time = Date.now();
      } else if (Date.now() - contour_time > 1000) {
        extract_contour(contour_corners)
        //document.getElementById("canvas_contour").style.display = "inline";
        found = true;
      }
    }
    if (corners != null) {
      contour_time = 0;
      contour_corners = corners;
      detectmrz(dstC3)
    }
  }
  return dstC1;
}


// Finds the intersection of two lines
// The lines are defined by (o1, p1) and (o2, p2).
function intersection(o1, p1, o2, p2) {
  let x = new cv.Point(o2.x - o1.x, o2.y - o1.y);
  let d1 = new cv.Point(p1.x - o1.x, p1.y - o1.y);
  let d2 = new cv.Point(p2.x - o2.x, p2.y - o2.y);

  let cross = d1.x*d2.y - d1.y*d2.x;
  if (Math.abs(cross) < /*EPS*/1e-8) {
    return null;
  }

  let t1 = (x.x * d2.y - x.y * d2.x) / cross;
  let ret = new cv.Point(o1.x + d1.x * t1, o1.y + d1.y * t1);
  return ret;
}

function detectcontour(orig) {
  let lines = new cv.Mat();
  let color = colorRed;
  let left = [];
  let right = [];
  let lower = [];
  let upper = [];
  let dsize = new cv.Size(width / contour_ratio, height / contour_ratio);
  cv.resize(orig, dstC5, dsize, 0, 0, cv.INTER_LINEAR);
  cv.medianBlur(dstC5, dstC5, 7);
  cv.Canny(dstC5, dstC5, 70, 200, 3);
  cv.HoughLinesP(dstC5, lines, 1, Math.PI / 180, 2, 40, 8);
  for (let i = 0; i < lines.rows; ++i) {
    lines.data32S[i * 4] = lines.data32S[i * 4] * contour_ratio;
    lines.data32S[i * 4 + 1] = lines.data32S[i * 4 + 1] * contour_ratio;
    lines.data32S[i * 4 + 2] = lines.data32S[i * 4 + 2] * contour_ratio;
    lines.data32S[i * 4 + 3] = lines.data32S[i * 4 + 3] * contour_ratio;
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
    if (diffX < 30) {
      if (a.x < width * 0.3) {
        left.push(i);
        cv.line(dstC1, a, b, colorRed, 3);
      }
      if (a.x > width * 0.7) {
        right.push(i)
        cv.line(dstC1, a, b, colorRed, 3);          
      }
    }
    // horizontal line
    if (diffY < 30) {
      if (a.y < height * 0.3) {
        lower.push(i)
        cv.line(dstC1, a, b, colorRed, 3);          
      }
      if (a.y > height * 0.7) {
        upper.push(i)
        cv.line(dstC1, a, b, colorRed, 3);          
      }
    }
  }
  // sort lower
  for (let i = 1; i < lower.length; i++) {
    let a = lower[i-1];
    let b = lower[i];
    let y_a = (lines.data32S[a * 4 + 1] + lines.data32S[a * 4 + 3]) / 2;
    let y_b = (lines.data32S[b * 4 + 1] + lines.data32S[b * 4 + 3]) / 2;
    if (y_b < y_a) {
      lower[i-1] = b;
      lower[i] = a;
    }
  }
  // sort upper
  for (let i = 1; i < upper.length; i++) {
    let a = upper[i-1];
    let b = upper[i];
    let y_a = (lines.data32S[a * 4 + 1] + lines.data32S[a * 4 + 3]) / 2;
    let y_b = (lines.data32S[b * 4 + 1] + lines.data32S[b * 4 + 3]) / 2;
    if (y_b > y_a) {
      upper[i-1] = b;
      upper[i] = a;
    }
  }
  // sort left
  for (let i = 1; i < left.length; i++) {
    let a = left[i-1];
    let b = left[i];
    let x_a = (lines.data32S[a * 4] + lines.data32S[a * 4 + 2]) / 2;
    let x_b = (lines.data32S[b * 4] + lines.data32S[b * 4 + 2]) / 2;
    if (x_b < x_a) {
      left[i-1] = b;
      left[i] = a;
    }
  }
  // sort right
  for (let i = 1; i < right.length; i++) {
    let a = right[i-1];
    let b = right[i];
    let x_a = (lines.data32S[a * 4] + lines.data32S[a * 4 + 2]) / 2;
    let x_b = (lines.data32S[b * 4] + lines.data32S[b * 4 + 2]) / 2;
    if (x_b > x_a) {
      right[i-1] = b;
      right[i] = a;
    }
  }

  //console.log("***********************************************");
  for (let i_ = 0; i_ < lower.length; ++i_) {
    let i = lower[i_];
    let lower_diff_x = lines.data32S[i * 4] - lines.data32S[i * 4 + 2];
    let lower_diff_y = lines.data32S[i * 4 + 1] - lines.data32S[i * 4 + 3];
    let lower_y = Math.min(lines.data32S[i * 4 + 1], lines.data32S[i * 4 + 3])
    let lower_rad = (Math.atan2(lower_diff_y, lower_diff_x) + 2 * Math.PI)  % Math.PI;
    if(lower_rad > (Math.PI / 2)) {
      lower_rad = lower_rad - Math.PI;
    }
    for (let j_ = 0; j_ < upper.length; ++j_) {
      let j = upper[j_];
      let upper_diff_x = lines.data32S[j * 4] - lines.data32S[j * 4 + 2];
      let upper_diff_y = lines.data32S[j * 4 + 1] - lines.data32S[j * 4 + 3];
      let upper_y = Math.max(lines.data32S[j * 4 + 1], lines.data32S[j * 4 + 3])
      let upper_rad = (Math.atan2(upper_diff_y, upper_diff_x) + 2 * Math.PI) % Math.PI;
      if(upper_rad > (Math.PI / 2)) {
        upper_rad = upper_rad - Math.PI;
      }
      for (let k_ = 0; k_ < left.length; ++k_) {
        let k = left[k_];
        let left_diff_x = lines.data32S[k * 4] - lines.data32S[k * 4 + 2];
        let left_diff_y = lines.data32S[k * 4 + 1] - lines.data32S[k * 4 + 3];
        let left_x = Math.min(lines.data32S[k * 4], lines.data32S[k * 4 + 2])
        let left_rad = (Math.atan2(left_diff_y, left_diff_x) + 2 * Math.PI) % Math.PI;
        if(left_rad > Math.PI) {
          left_rad = left_rad - Math.PI;
        }
        // left line not between lower and upper
        if(lines.data32S[k * 4 + 1] < lower_y || lines.data32S[k * 4 + 1] > upper_y ||
           lines.data32S[k * 4 + 3] < lower_y || lines.data32S[k * 4 + 3] > upper_y) {
             continue;
        }
        for (let l_ = 0; l_ < right.length; ++l_) {
          let l = right[l_];
          let right_diff_x = lines.data32S[l * 4] - lines.data32S[l * 4 + 2];
          let right_diff_y = lines.data32S[l * 4 + 1] - lines.data32S[l * 4 + 3];
          let right_x = Math.max(lines.data32S[l * 4], lines.data32S[l * 4 + 2])
          let right_rad = (Math.atan2(right_diff_y, right_diff_x) + 2 * Math.PI) % Math.PI;
          if(right_rad > Math.PI) {
            right_rad = right_rad - Math.PI;
          }

          // right line not between lower and upper
          if(lines.data32S[l * 4 + 1] < lower_y || lines.data32S[l * 4 + 1] > upper_y ||
            lines.data32S[l * 4 + 3] < lower_y || lines.data32S[l * 4 + 3] > upper_y) {
              continue;
          }
          // upper line not between left and right
          if(lines.data32S[j * 4] < left_x || lines.data32S[j * 4] > right_x ||
            lines.data32S[j * 4 + 2] < left_x || lines.data32S[j * 4 + 2] > right_x) {
              continue;
          }
          // lower line not between left and right
          if(lines.data32S[i * 4] < left_x || lines.data32S[i * 4] > right_x ||
            lines.data32S[i * 4 + 2] < left_x || lines.data32S[i * 4 + 2] > right_x) {
              continue;
          }

          //console.log("___" + lower_rad + "_" + upper_rad + "_" + left_rad + "_" + right_rad);
          
          let rot1 = ((lower_rad + upper_rad ) % Math.PI) / 2;
          let rot2 = ((left_rad + right_rad - Math.PI) % Math.PI) / 2;
          let rot = Math.abs(rot1 - rot2)
          if (rot > 0.05) {
            continue;
          }          
          let alpha1 = ((lower_rad - upper_rad ) % Math.PI) ;
          let alpha2 = ((left_rad - right_rad) % Math.PI) ;
          
          let lower_a = new cv.Point(lines.data32S[i * 4], lines.data32S[i * 4 + 1]);
          let lower_b = new cv.Point(lines.data32S[i * 4 + 2], lines.data32S[i * 4 + 3]);
          let upper_a = new cv.Point(lines.data32S[j * 4], lines.data32S[j * 4 + 1]);
          let upper_b = new cv.Point(lines.data32S[j * 4 + 2], lines.data32S[j * 4 + 3]);
          let left_a = new cv.Point(lines.data32S[k * 4], lines.data32S[k * 4 + 1]);
          let left_b = new cv.Point(lines.data32S[k * 4 + 2], lines.data32S[k * 4 + 3]);
          let right_a = new cv.Point(lines.data32S[l * 4], lines.data32S[l * 4 + 1]);
          let right_b = new cv.Point(lines.data32S[l * 4 + 2], lines.data32S[l * 4 + 3]);                
          let lower_left = intersection(lower_a, lower_b, left_a, left_b);
          let lower_right = intersection(lower_a, lower_b, right_a, right_b);
          let upper_left = intersection(upper_a, upper_b, left_a, left_b);
          let upper_right = intersection(upper_a, upper_b, right_a, right_b);

          // check 2/3 ration
          let width_lower = lower_right.x - lower_left.x
          let height_left = upper_left.y - lower_left.y;
          let len_x = Math.max(lower_right.x - lower_left.x, upper_right.x - upper_left.x) / Math.cos(alpha1);
          let len_y = Math.max(upper_left.y - lower_left.y, upper_right.y - lower_right.y) / Math.cos(alpha2);
          let ratio = len_x / len_y;
          //console.log("length:" + len_x);
          //console.log("height:" + len_y);
          //console.log("ratio:" + ratio);
          // angle up to 30° ok
          if (ratio < 1.2 || ratio > 1.8) {
            continue;
          }

          cv.line(dstC1, lower_a, lower_b, colorGreen, 3);
          cv.line(dstC1, upper_a, upper_b, colorGreen, 3);
          cv.line(dstC1, left_a, left_b, colorGreen, 3);
          cv.line(dstC1, right_a, right_b, colorGreen, 3);
          cv.line(dstC1, lower_left, lower_right, colorGreen, 1);
          cv.line(dstC1, lower_right, upper_right, colorGreen, 1);
          cv.line(dstC1, upper_right, upper_left, colorGreen, 1);
          cv.line(dstC1, upper_left, lower_left, colorGreen, 1);

          if ((width_lower < contour_width - 10) && (height_left < contour_height - 10)) {
            continue;
          }
          else if (width_lower > contour_width) {
            contour_width = width_lower;
          }
          else if (height_left > contour_height) {
            contour_height = height_left;
          }

          cv.imshow('canvas_contour', src);

          let xf = video.videoWidth / width;
          let yf = video.videoHeight / height;
          let roi_width = len_x * xf;
          let roi_height = len_y * yf;

          lines.delete();

          return [ // edges original image
                  lower_left.x * xf, lower_left.y * yf,
                  lower_right.x * xf, lower_right.y * yf,
                  upper_left.x * xf, upper_left.y * yf,
                  upper_right.x * xf, upper_right.y * yf,
                  // roi edges
                  0, 0, roi_width, 0, 0, roi_height, roi_width, roi_height
                ];
        }
      }
    }
  }
  lines.delete();
  return null;
}

function extract_contour(corners) {
  let roi = corners.splice(8)
  let srcTri = cv.matFromArray(4, 1, cv.CV_32FC2, corners);
  let dstTri = cv.matFromArray(4, 1, cv.CV_32FC2, roi);
  let M = cv.getPerspectiveTransform(srcTri, dstTri);
  let src = cv.imread('canvas_contour')
  let dst = new cv.Mat();
  let dsize = new cv.Size(roi[2], roi[5]);
  cv.warpPerspective(src, dst, M, dsize, cv.INTER_LINEAR, cv.BORDER_CONSTANT, new cv.Scalar());
  cv.imshow('canvas_contour', dst);

  return true;
}

function detectmrz(dstC3) {
  cv.Canny(dstC3, dstC4, 150, 200, 3, true);
  cv.morphologyEx(dstC4, dstC3, cv.MORPH_BLACKHAT, rectKernel);
  cv.morphologyEx(dstC3, dstC3, cv.MORPH_CLOSE, rectKernel);
  cv.threshold(dstC3, dstC3, 0, 255, cv.THRESH_BINARY | cv.THRESH_OTSU);
  
  const contoursCard = new cv.MatVector();
  const contoursRoi = new cv.MatVector();
  const hierarchy = new cv.Mat();
  // RETR_EXTERNAL, RETR_LIST
  cv.findContours(dstC3, contoursRoi, hierarchy, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE);
    
  let foundRoi = false;
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
    let area = cv.contourArea(cnt, false);
    let rectArea = rect.width * rect.height;
    let extent = area / rectArea;
    if (extent < 0.5) {
      continue;
    }
    foundRoi = true;
    minX = Math.min(minX, rect.x - 20);
    maxX = Math.max(maxX, rect.x + rect.width + 20);
    minY = Math.min(minY, rect.y);
    maxY = Math.max(maxY, rect.y + rect.height);
  }
  if (!foundRoi) {
    return false;
  }
  let minP = new cv.Point(minX - 10, minY - 10);
  let maxP = new cv.Point(maxX + 20, maxY + 10);
  //console.log("******** " + minX + ", " + minY + ", " + maxX + ", " + maxY + ", ");
  cv.rectangle(dstC1, minP, maxP, colorRed, 2);
  
  let x = (minX - 10) * video.videoWidth / width;
  let w = (maxX - minX  + 20) * video.videoWidth / width;
  let y = (minY - 5) * video.videoHeight / height;
  let h = (maxY - minY  + 10) * video.videoHeight / height;
  let rect = new cv.Rect(x, y, w, h);
  if (rect == null) {
    return false;
  }
  let roi = src.roi(rect);
  let roi2 = new cv.Mat();
  cv.cvtColor(roi, roi2, cv.COLOR_RGB2GRAY);
  cv.imshow("canvas_roi", roi2);
  let canvasRoi = document.getElementById("canvas_roi");
  let imgData = canvasRoi.toDataURL("image/jpeg", 0.8);
  let media = imgData.substr(23); // strip data:image/jpeg;base64,
  
  fetch('/mrz/api/v1/analyze_image', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({file_data: media}),
  })
  .then((response) => response.json())
  .then((data) => {
    console.log("###################################");
    console.log(data);
    console.log("###################################");
    var pretty = JSON.stringify(data, undefined, 4);
    if (data.type) {
      document.getElementById('mrztext').value = pretty;
      cv.rectangle(dstC1, minP, maxP, colorGreen, 2);
    }
  })
  .catch((error) => {
    console.log("###################################");
    console.log(error);
    console.log("###################################");
  });
  roi.delete();
}
