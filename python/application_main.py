import multiprocessing
import tkinter
import application_task_0

Q = multiprocessing.Queue(64)

def Task_GUI(Q: multiprocessing.Queue):
    app = tkinter.Tk()
    app.geometry('200x100+622+174')
    app["background"] = "#152c30"
    app.attributes("-alpha", 0.85)
    app.resizable(0, 0)
    app.wm_attributes('-topmost', 1)
    btn_0 = tkinter.Button(app, text="退出", command=lambda: Q.put("q"))
    btn_1 = tkinter.Button(app, text="拍照", command=lambda: Q.put("s"))
    btn_0.pack()
    btn_1.pack()
    app.mainloop()

if __name__ == "__main__":
    p1 = multiprocessing.Process(target=Task_GUI, args=(Q,))
    p2 = multiprocessing.Process(target=application_task_0.Task_0, args=(Q,))
    p1.start()
    p2.start()