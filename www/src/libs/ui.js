"use strict";

TemplateJS.View.regCustomElement("X-SLIDER", new TemplateJS.CustomElement(
		function(elem,val) {
			var range = elem.querySelector("input[type=range]");
			var number = elem.querySelector("input[type=number]");
			var mult = parseFloat(elem.dataset.mult);
			var fixed = parseInt(elem.dataset.fixed);
			var toFixed = function(v) {
				if (!isNaN(fixed)) return parseFloat(v).toFixed(fixed);
				else return v;
			};
			if (!range) {
				range = document.createElement("input");
				range.setAttribute("type","range");
				number = document.createElement("input");
				number.setAttribute("type","number");
				number.setAttribute("step",mult);
				var env1 = document.createElement("div");
				var env2 = document.createElement("div");
				var min = parseFloat(elem.dataset.min);
				var max = parseFloat(elem.dataset.max);
				var rmin = Math.floor(min/mult);
				var rmax = Math.floor(max/mult);
				range.setAttribute("min",rmin);
				range.setAttribute("max",rmax);
				range.addEventListener("input",function() {
					var v = parseInt(this.value);
					var val = v * mult;
					number.value = toFixed(val);
					elem.dispatchEvent(new Event("change"));
				});
				number.addEventListener("change", function() {
					var v = parseFloat(this.value);
					var val = v / mult;
					range.value = val;
					elem.dispatchEvent(new Event("change"));
				});				
				env1.appendChild(range);
				env2.appendChild(number);
				elem.appendChild(env1);
				elem.appendChild(env2);
			}
			range.value = val / mult;
			number.value = toFixed(val);

			
		},
		function(elem) {
			var number = elem.querySelector("input[type=number]");
			if (number) return parseFloat(number.valueAsNumber);
			else return 0;
			
		},
		function(elem,attrs) {
			
		}
));


TemplateJS.View.regCustomElement("X-CHECKBOX", new TemplateJS.CustomElement(
		function(elem,val) {
			var z = elem.querySelector("[data-name=inner");
			if (z == null) {
				z = document.createElement("span");
				z.setAttribute("data-name","inner");				
				elem.insertBefore(z, elem.firstChild);
				var w = document.createElement("span");
				z.appendChild(w);
				
				elem.addEventListener("click", function() {
					elem.classList.toggle("checked");
					elem.dispatchEvent(new Event("change"));
				});
			}
			elem.classList.toggle("checked", val);
		},
		function(elem) {
			return elem.classList.contains("checked");
		}
		
));

TemplateJS.View.regCustomElement("X-SWITCH", new TemplateJS.CustomElement(
		function(elem, val) {
			if (elem.dataset.separator) {
				var arr = elem.dataset.value.split(elem.dataset.separator);
				elem.hidden = arr.indexOf(val) == -1;
			} else {
				elem.hidden = (elem.dataset.value!=val);
			}
		},
		function(elem) {
			return undefined;
		}
));

var x_select_map = new WeakMap; 

TemplateJS.View.regCustomElement("X-SELECT", new TemplateJS.CustomElement(
		function(elem, val) {
			var input = elem.querySelector("input");
			var box = elem.querySelector("select");
			if (!input) {
				x_select_map.set(elem,{"value":null,label:"", cb:function(){return [];}});
				input = document.createElement("input");
				input.setAttribute("type","text");
				box = document.createElement("select");
				box.setAttribute("size",8);
				box.setAttribute("tabindex","-1");
				elem.appendChild(input);
				elem.appendChild(box);
				box.hidden = true;
				async function populate(text) {
					let info = x_select_map.get(elem);
					let res = await info.cb(true, text);
					if (!res || !res.length) {
						box.hidden = true;
						info.value = null;
					} else {
						box.hidden = false;
						while (box.firstChild) box.removeChild(box.firstChild);
						res.forEach(function(v){
							let opt = document.createElement("option");
							opt.value = v[0];
							opt.textContent = v[1];
							box.appendChild(opt);
						});
					}
					
				}
				input.addEventListener("input", function() {
					populate(this.value.trim()).then(()=>{
						let info = x_select_map.get(elem);
						if (box.options.length) {
							info.value = box.options[0].value;
							info.label = box.options[0].textContent;
						}
					});
				});
				
				function looseFocus() {
					let info = x_select_map.get(elem);
					let inp = input.value.trim();
					if (info.value === null && inp) {
						if (box.options[0]) {
							info.value = box.options[0].value;
							info.label = input.value = box.options[0].textContent;

						} else {
							input.value = "";							
						}
					} else if (!inp) {
						input.value = "";					
						info.value = null;
						info.label="";
					} else {
	                    input.value = info.label;					
     				}
					box.hidden = true;					
				}
				input.addEventListener("blur", function(ev){
					if (ev.relatedTarget != box) looseFocus();
				});
				box.addEventListener("blur", function(ev){
					if (ev.relatedTarget != input) looseFocus();
				});
				input.addEventListener("keydown", function(ev){
					if (ev.code == "ArrowDown") {
						ev.preventDefault();
						ev.stopPropagation();
						if(!box.hidden && box.options.length>0) {
							box.focus();
							box.selectedIndex = 0;
							let info = x_select_map.get(elem);
							info.value = box.options[0].value;
							info.label = input.value = box.options[0].textContent;
						} else {
							while (box.firstChild) box.removeChild(box.firstChild);
							box.hidden = false;
							box.focus();
							populate(input.value.trim()).then(()=>{
								if (box.options.length) {
									box.selectedIndex = 0;
									let info = x_select_map.get(elem);
									info.value = box.options[0].value;
									info.label = input.value = box.options[0].textContent;
								} else {
									input.focus();
								}
							});						
						}
					}
				});			
				box.addEventListener("change", function() {
					let info = x_select_map.get(elem);
					let idx = box.selectedIndex;
					if (idx < 0) {
						input.value = "";
						info.value = null;
					} else {
						info.value = box.options[idx].value;
						info.label = input.value = box.options[idx].textContent;
					}
				});	
				box.addEventListener("keydown", function(ev) {
					if (ev.code == "ArrowUp" && box.selectedIndex<1) {
						ev.preventDefault();
						ev.stopPropagation();
						box.selectedIndex = -1;
						input.focus();
						input.select();
					} else if (ev.code == "Escape" || ev.code == "Enter") {
						ev.preventDefault();
						ev.stopPropagation();
						input.focus();
						input.select();
						box.hidden = true;
					}
				})				
						
			};
			let info = x_select_map.get(elem);
			var newvalue;
			if (Array.isArray(val)) {
				if (val.length == 0) return;
				newvalue = val[0];
				if (typeof val[1] == "function") {
					info.cb = val[1];
				} else if (Array.isArray(typeof val[1])) {
					let items = val[1];
					info.cb = function(srch, txt) {
						if (srch) {
							return items.filter(x=>x[1].indexOf(txt) != -1);
						} else {
							let fnd = items.find(x=>x[0] == txt);
							if (fnd) return fnd[1];
							else return txt;
						}
					};
				}
				
			} else {
				newvalue = val;
			}
			info.value = newvalue;
			(async function(){
				info.label = input.value = await info.cb(false, newvalue);
			})();		
		},
		function(elem) {
			let info = x_select_map.get(elem);
			if (info) return info.value;
			else return undefined;
		}
));
